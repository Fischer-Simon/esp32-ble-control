/*
 * Part of Esp32BleControl a firmware to allow ESP32 remote control over BLE.
 * Copyright (C) 2024  Simon Fischer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "LedString.h"

#include <chrono>

std::vector<bool> LedString::colorBufferUpdated;
std::vector<RgbwColor> LedString::colorBuffer;
std::mutex LedString::colorBufferMutex;

LedString::LedString(const std::shared_ptr<KeyValueStore>& keyValueStore, const std::shared_ptr<Led::ColorManager>& colorManager, std::string description,
                     float defaultBrightness, const Led::HslwColor& primaryColor,
                     led_index_t ledCount)
    : LedView{keyValueStore, colorManager, std::move(description), defaultBrightness, primaryColor},
      m_ledCount{ledCount},
      m_manualAnimationReleaseTime{std::chrono::system_clock::now()} {
    m_leds.resize(ledCount);
    ensureColorBufferSize(ledCount);
    m_updateTimer = xTimerCreate("led_update", pdMS_TO_TICKS(16), pdTRUE, this, &updateTimerFn);
    if (m_updateTimer == nullptr) {
        throw std::runtime_error("Failed to create LED update timer");
    }
    xTimerStart(m_updateTimer, 0);
}

LedString::~LedString() {
    xTimerDelete(m_updateTimer, portMAX_DELAY);
}

void LedString::addAnimation(std::unique_ptr<AnimationConfig> config) {
    const auto now = std::chrono::system_clock::now();
    if (config->leds.empty()) {
        config->leds.resize(m_ledCount);
        for (led_index_t i = 0; i < m_ledCount; i++) {
            config->leds[i] = i;
        }
    }

    if (config->halfCycles < 0) {
        config->halfCycles *= -1;
        std::reverse(config->leds.begin(), config->leds.end());
    }

    if (!config->targetColorStr.empty()) {
        config->targetColor = m_colorManager->parseColor(config->targetColorStr, getPrimaryColor());
        config->targetColorStr.clear();
    }
    config->targetColor.dim(getBrightness());

    std::vector<Animation::PerLed> perLedData;
    perLedData.reserve(config->leds.size());

    const auto startTime = now + config->startDelay;
    auto endTime = startTime;
    float cosA = cosf(config->modelLocation.angleRad);
    float sinA = sinf(config->modelLocation.angleRad);
    for (size_t i = 0; i < config->leds.size(); i++) {
        Animation::duration ledDuration = config->ledDuration.eval(config->leds.size());
        Animation::duration ledDelay;
        uint16_t ledBrightnessFactor{65535};
        if (config->animationType == AnimationType::Wave3D) {
            auto ledPosition = getLedPosition(config->leds[i]);

            float x = getPosition().x + ledPosition.x;
            float y = getPosition().y + ledPosition.y;
            float z = getPosition().z + ledPosition.z;

            float localX = x * cosA - y * sinA;
            float localY = x * sinA + y * cosA;

            x = localX + config->modelLocation.x - std::get<0>(config->startPos);
            y = localY + config->modelLocation.y - std::get<1>(config->startPos);
            z = z + config->modelLocation.z - std::get<2>(config->startPos);

            float distance = std::sqrt(x * x + y * y + z * z);
            ledDelay = config->ledDelay.eval(config->leds.size(), distance);
            if (config->range > 0) {
                ledBrightnessFactor = static_cast<uint16_t>(65535 * Easing::easeOutSine(std::max(0.f, 1 - (distance / config->range))));
            }
        } else {
            ledDelay = config->ledDelay.eval(config->leds.size(), static_cast<float>(i));
        }

        auto ledEndTime = startTime + ledDelay + ledDuration;
        endTime = std::max(endTime, ledEndTime);

        perLedData.emplace_back(
            config->leds[i],
            ledDuration,
            ledDelay,
            ledBrightnessFactor
        );
    }

    if (config->halfCycles % 2 == 1 && config->blending != Led::Blending::Add) {
        auto animationTargetColor = config->targetColor;
        animationTargetColor.setBrightness(1);
        for (auto& ledView: config->affectedLedViews) {
            ledView->updateAnimationTargetColor(config->targetColor, endTime);
        }
        updateAnimationTargetColor(config->targetColor, endTime);
    }

    std::unique_lock<std::mutex> animationsLock{m_animationsMutex};

    auto it = config->blending != Led::Blending::Add ? m_animations.begin() : m_animations.end();
    while (it != m_animations.end()) {
        const auto& animation = it->get();
        if (endTime < animation->endTime || animation->blending == Led::Blending::Add) {
            break;
        }
        ++it;
    }
    m_animations.emplace(
        it,
        std::unique_ptr<Animation>(new Animation{
            .blending = config->blending,
            .easing = config->easing,
            .targetColor = config->targetColor,
            .startTime = startTime,
            .endTime = endTime,
            .halfCycles = config->halfCycles,
            .leds = std::move(perLedData),
        }));
}

void LedString::endAllAnimations() const {
    auto now = std::chrono::system_clock::now();
    for (const auto& animation: m_animations) {
        animation->endTime = now;
        for (const auto& ledViewWeak: animation->affectedLedViews) {
            auto ledView = ledViewWeak.lock();
            if (!ledView) {
                continue;
            }
            ledView->setCurrentAnimationEnd(now);
        }
    }
}

void LedString::update() {
    std::unique_lock<std::mutex> colorBufferLock{colorBufferMutex};
    for (led_index_t i = 0; i < m_ledCount; i++) {
        colorBuffer[i] = m_leds[i].currentBaseColor;
        colorBufferUpdated[i] = false;
    }

    const auto now = std::chrono::system_clock::now();
    std::unique_lock<std::mutex> animationsLock{m_animationsMutex};
    bool anyAnimationActive = false;
    for (auto it = m_animations.begin(); it != m_animations.end();) {
        auto& animation = *it;
        if (now < animation->startTime) {
            it++;
            continue;
        }
        auto targetColor = animation->targetColor.toRgbwColor();
        bool animationFinishes = animation->endTime <= now;
        for (size_t i = 0; i < animation->leds.size(); i++) {
            Animation::PerLed& led = animation->leds[i];

            auto ledStartTime = animation->startTime + led.ledDelay;
            if (now < ledStartTime) {
                continue;
            }
            auto animationTimeRunning = std::chrono::duration_cast<Led::Animation::duration>(now - ledStartTime);

            led_index_t ledIndex = led.ledIndex;
            auto& ledColor = colorBuffer[ledIndex];
            float animationProgress = led.ledDuration.count() > 0 ? std::min(
                1.f, static_cast<float>(animationTimeRunning.count()) /
                     static_cast<float>(led.ledDuration.count())
            ) : 1.f;

            animationProgress *= static_cast<float>(animation->halfCycles);
            const int animationCycle = static_cast<int>(animationProgress);
            animationProgress -= static_cast<float>(animationCycle);
            if (animationCycle % 2) {
                animationProgress = 1 - animationProgress;
            }

            float blendValue = animation->easing(animationProgress) * (static_cast<float>(led.ledBrightnessFactor) / 65535.f);
            switch (animation->blending) {
                case Led::Blending::Blend:
                    ledColor = RgbwColor::LinearBlend(ledColor, targetColor, blendValue);
                    break;
                case Led::Blending::Add: {
                    auto addColor = RgbwColor::LinearBlend({0, 0, 0}, targetColor, blendValue);
                    ledColor = RgbwColor(
                        std::min(255, ledColor.R + addColor.R),
                        std::min(255, ledColor.G + addColor.G),
                        std::min(255, ledColor.B + addColor.B),
                        std::min(255, ledColor.W + addColor.W)
                    );
                }
                break;
            }
            colorBufferUpdated[ledIndex] = true;
            if (animationFinishes && animation->blending != Led::Blending::Add) {
                m_leds[ledIndex].currentBaseColor = ledColor;
            }

            anyAnimationActive = true;
        }

        if (animationFinishes) {
            it = m_animations.erase(it);
        } else {
            ++it;
        }
    }
    animationsLock.unlock();

    if (!anyAnimationActive) {
        return;
    }

    bool shouldShowLeds = false;
    for (led_index_t i = 0; i < m_ledCount; i++) {
        if (!colorBufferUpdated[i]) {
            continue;
        }
        setLedColor(i, colorBuffer[i]);
        shouldShowLeds = true;
    }
    if (shouldShowLeds) {
        showLeds();
    }
}

void LedString::ensureColorBufferSize(led_index_t size) {
    std::unique_lock<std::mutex> ledColorBufferLock{colorBufferMutex};
    if (colorBuffer.size() < size) {
        colorBuffer.resize(size);
        colorBufferUpdated.resize(size);
    }
}
