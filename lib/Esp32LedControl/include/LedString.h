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

#pragma once

#include <list>
#include <mutex>
#include <NeoPixelBus.h>
#include <random>

#include "LedView.h"
#include "Led/Animation.h"
#include "Led/Led.h"

class LedString : public LedView {
public:
    using led_index_t = Led::Led::index_t;

    LedString(const std::shared_ptr<KeyValueStore>& keyValueStore, const std::shared_ptr<Led::ColorManager>& colorManager, std::string description, float defaultBrightness,
              const Led::HslwColor& primaryColor, led_index_t ledCount);

    ~LedString() override;

    Led::Led::index_t getLedCount() const override {
        return m_ledCount;
    }

    RgbwColor getLedColor(Led::Led::index_t i) const override {
        if (i >= m_ledCount) {
            return {0};
        }
        return m_leds[i].currentBaseColor;
    }

    bool isPositionAware() const override {
        return true;
    }

    void setPosition(const Led::Position position) {
        m_position = position;
    }

    Led::Position getPosition() const override {
        return m_position;
    }

    void setLedPosition(const Led::Led::index_t index, const Led::Position position) {
        if (index >= m_ledCount) {
            return;
        }
        m_leds[index].position = position;
    }

    Led::Position getLedPosition(Led::Led::index_t index) const override {
        if (index >= m_ledCount) {
            return {0, 0, 0};
        }
        return m_leds[index].position;
    }

    void addAnimation(std::unique_ptr<AnimationConfig> config) override;

    void endAllAnimations() const;

    void update();

protected:
    virtual void setLedColor(led_index_t index, RgbwColor color) = 0;

    virtual void showLeds() = 0;

private:
    static void ensureColorBufferSize(led_index_t size);

    static void updateTimerFn(TimerHandle_t timer) {
        static_cast<LedString*>(pvTimerGetTimerID(timer))->update();
    }

    Led::Position m_position;

    led_index_t m_ledCount;
    std::vector<Led::Led> m_leds;

    std::chrono::system_clock::time_point m_manualAnimationReleaseTime;
    std::list<std::unique_ptr<Led::Animation> > m_animations;
    std::mutex m_animationsMutex;

    TimerHandle_t m_updateTimer;

    static std::vector<bool> colorBufferUpdated;
    static std::vector<RgbwColor> colorBuffer;
    static std::mutex colorBufferMutex;
};
