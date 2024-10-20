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
std::vector<Rgb48Color> LedString::colorBuffer;
std::mutex LedString::colorBufferMutex;

LedString::LedString(std::string description, float defaultBrightness, const HslColor& primaryColor, led_index_t ledCount)
    : LedView{std::move(description), defaultBrightness, primaryColor},
      m_ledCount{ledCount} {
    m_leds.resize(ledCount);
    ensureColorBufferSize(ledCount);
    m_updateTimer = xTimerCreate("led_update", pdMS_TO_TICKS(16), pdTRUE, this, &updateTimerFn);
    if (m_updateTimer == nullptr) {
        throw std::runtime_error("Failed to create LED update timer");
    }
    xTimerStart(m_updateTimer, 0);
}

LedString::~LedString() {
    xTimerDelete(m_updateTimer, pdMS_TO_TICKS(200));
}

void LedString::addAnimation(AnimationConfig config) {
    if (config.leds.empty()) {
        config.leds.resize(m_ledCount);
        for (led_index_t i = 0; i < m_ledCount; i++) {
            config.leds[i] = i;
        }
    }

    if (config.ledDelay < std::chrono::milliseconds(0)) {
        config.ledDelay *= -1;
        std::reverse(config.leds.begin(), config.leds.end());
    }

    const std::default_random_engine::result_type randomSeed = esp_random();
    const auto startTime = std::chrono::system_clock::now() + config.startDelay;
    const auto endTime = startTime + (config.leds.size() * config.ledDelay) + config.ledDuration;

    if (config.halfCycles % 2 == 1 && config.blending != Led::Blending::Add) {
        for (auto& ledView: config.affectedLedViews) {
            ledView->updateAnimationTargetColor(config.targetColor, endTime);
        }
    }

    std::unique_lock<std::mutex> animationsLock{m_animationsMutex};
    m_animations.emplace_back(Animation{
        .randomSeed = randomSeed,
        .blending = config.blending,
        .easing = config.easing,
        .targetColor = HslColor{config.targetColor.H, config.targetColor.S, config.targetColor.L * config.targetBrightness},
        .startTime = startTime,
        .ledDuration = config.ledDuration,
        .ledDelay = config.ledDelay,
        .halfCycles = config.halfCycles,
        .endTime = endTime,
        .leds = std::move(config.leds),
    });
    std::sort(m_animations.begin(), m_animations.end(), [](const Animation& lhs, const Animation& rhs) {
        return lhs.endTime < rhs.endTime;
    });
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
    for (const auto& animation: m_animations) {
        if (now < animation.startTime) {
            continue;
        }
        auto animationTimeRunning = std::chrono::duration_cast<Led::Animation::duration>(now - animation.startTime);
        bool animationFinishes = now > animation.endTime;
        for (size_t i = 0; i < animation.leds.size(); i++) {
            if (animationTimeRunning < std::chrono::seconds(0)) {
                break;
            }

            led_index_t ledIndex = animation.leds[i];
            auto& ledColor = colorBuffer[ledIndex];
            float animationProgress = std::min(
                1.f, static_cast<float>(animationTimeRunning.count()) /
                     static_cast<float>(animation.ledDuration.count())
            );

            animationProgress *= static_cast<float>(animation.halfCycles);
            const int animationCycle = static_cast<int>(animationProgress);
            animationProgress -= static_cast<float>(animationCycle);
            if (animationCycle % 2) {
                animationProgress = 1 - animationProgress;
            }

            float blendValue = animation.easing(animationProgress);
            switch (animation.blending) {
                case Led::Blending::Blend:
                    ledColor = Rgb48Color::LinearBlend(ledColor, animation.targetColor, blendValue);
                    break;
                case Led::Blending::Add: {
                    auto addColor = Rgb48Color::LinearBlend({0, 0, 0}, animation.targetColor, blendValue);
                    ledColor = Rgb48Color(
                        std::min(65535, ledColor.R + addColor.R),
                        std::min(65535, ledColor.G + addColor.G),
                        std::min(65535, ledColor.B + addColor.B)
                    );
                }
                break;
                case Led::Blending::Overwrite:
                    ledColor = Rgb48Color::LinearBlend({0, 0, 0}, animation.targetColor, blendValue);
                    break;
            }
            colorBufferUpdated[ledIndex] = true;
            if (animationFinishes) {
                m_leds[ledIndex].currentBaseColor = ledColor;
            }

            animationTimeRunning -= animation.ledDelay;
            anyAnimationActive = true;
        }
    }
    m_animations.erase(std::remove_if(m_animations.begin(), m_animations.end(),
                                      [&now](const Led::Animation& animation) {
                                          return now > animation.endTime;
                                      }), m_animations.end());
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
