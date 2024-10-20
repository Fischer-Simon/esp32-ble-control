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

class LedView;

namespace Led {
struct Animation {
    using time_point = std::chrono::system_clock::time_point;
    using duration = std::chrono::milliseconds;

    /**
     * The random seed used to calculate per LED random values.
     */
    std::default_random_engine::result_type randomSeed;

    /**
     * The blending used for the animation. Note that blending is done in RGB colorspace.
     */
    Blending blending;

    /**
     * Easing function to use.
     */
    ease_func_t easing;

    /**
     * For @link blending = @link Blending::Blend or @link Blending::Overwrite:
     * Color the LEDs will have after the animation has finished.
     *
     * For @link blending = @link Blending::Add:
     * Peak color value added to the LEDs color.
     */
    HslColor targetColor;

    /**
     * Time the animation starts at.
     */
    time_point startTime;

    /**
     * Duration of one LED animation.
     */
    duration ledDuration;

    /**
     * Animation start delay relative to the LED index.
     * So the start delay of the fourth LED (with index 3) is 3 * ledDelay
     */
    duration ledDelay;

    /**
     * Number of cycles the animation runs. One half cycle plays the animation forward once. Two play the animation
     * once forward and once reverse (so the LED color at the end of the animation will be the same as at the start).
     * Three play the animation forward, reverse and forward again. And so on.
     * The complete LED animation takes @link ledDuration time. So for three half cycles each one takes a third of that.
     */
    uint8_t halfCycles;

    /**
     * Calculated time based on @link startTime, @link ledDuration and @link ledDelay the animation will end at.
     */
    time_point endTime;

    /**
     * LED indices affected by this animation.
     */
    std::vector<Led::index_t> leds;

    std::vector<std::weak_ptr<LedView>> affectedLedViews;

    static duration parseDuration(const std::string& durationStr);
};
}
