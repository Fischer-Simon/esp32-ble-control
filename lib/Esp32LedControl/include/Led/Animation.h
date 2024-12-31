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

#include <memory>
#include "../Easing.h"
#include "Blending.h"
#include "Led.h"

#include <chrono>
#include <random>
#include <NeoPixelBus.h>

#include "HslwColor.h"

class LedView;

namespace Led {
struct Animation {
    using time_point = std::chrono::system_clock::time_point;
    // using duration = std::chrono::milliseconds;
    using duration = std::chrono::duration<uint16_t, std::centi>;

    static constexpr duration durationFromMs(uint16_t ms) {
        return duration{ms / 10};
    }

    struct RndDuration {
        duration minValue{durationFromMs(0)};
        duration maxValue{durationFromMs(0)};

        /**
        * Indicates the duration is given per led instead of the complete animation.
        */
        bool perLed{true};

        /**
        * Indicates the duration is a global delay value instead of a value increasing with each LED.
        */
        bool delayIsGlobal{false};

        RndDuration() = default;

        RndDuration(duration minValue_, duration maxValue_, bool perLed_, bool delayIsGlobal_)
            : minValue{minValue_}, maxValue{maxValue_}, perLed{perLed_}, delayIsGlobal{delayIsGlobal_} {
        }

        duration eval(int ledCount, float offset = 1) const {
            if (delayIsGlobal) {
                offset = 1;
            }
            duration ret;
            if (maxValue == minValue) {
                ret = duration{static_cast<uint16_t>(offset * static_cast<float>(minValue.count()))};
            } else {
                ret = duration{
                    static_cast<uint16_t>(offset * static_cast<float>(
                                              minValue.count() + esp_random() % (maxValue.count() - minValue.count())))
                };
            }
            if (!perLed) {
                ret /= ledCount;
            }
            return ret;
        }
    };

    struct PerLed {
        /**
        * Mapped LED index.
        */
        Led::index_t ledIndex;

        /**
        * Per LED animation duration.
        */
        duration ledDuration;

        /**
        * Per LED delay relative to the @link startTime of the animation.
        */
        duration ledDelay;

        /**
        * Factor for maximum animation brightness (65535 = 1.0)
        */
        uint16_t ledBrightnessFactor;

        PerLed(Led::index_t ledIndex_, duration ledDuration_, duration ledDelay_, uint16_t ledBrightnessFactor_)
            : ledIndex{ledIndex_},
              ledDuration{ledDuration_},
              ledDelay{ledDelay_},
              ledBrightnessFactor{ledBrightnessFactor_} {
        }
    };

    /**
     * The blending used for the animation. Note that blending is done in RGB colorspace.
     */
    Blending blending;

    /**
     * Easing function to use.
     */
    ease_func_t easing;

    /**
     * For @link blending = @link Blending::Blend:
     * Color the LEDs will have after the animation has finished.
     *
     * For @link blending = @link Blending::Add:
     * Peak color value added to the LEDs color.
     */
    HslwColor targetColor;

    float targetBrightness;

    /**
     * Time the animation starts at.
     */
    time_point startTime;

    /**
     * Calculated time based on @link startTime, @link ledDuration and @link ledDelay the animation will end at.
     */
    time_point endTime;

    /**
     * Number of cycles the animation runs. One half cycle plays the animation forward once. Two play the animation
     * once forward and once reverse (so the LED color at the end of the animation will be the same as at the start).
     * Three play the animation forward, reverse and forward again. And so on.
     * The complete LED animation takes @link ledDuration time. So for three half cycles each one takes a third of that.
     */
    int8_t halfCycles;

    /**
     * Per LED animation information
     */
    std::vector<PerLed> leds;

    std::vector<std::weak_ptr<LedView> > affectedLedViews;

    static RndDuration parseDuration(std::string durationStr);
};
}
