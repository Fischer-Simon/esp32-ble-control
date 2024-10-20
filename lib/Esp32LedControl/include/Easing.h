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

#include <cmath>
#include <string>
#include <array>

typedef float(* ease_func_t)(float);
typedef uint8_t ease_index_t;

extern const std::array<ease_func_t, 30> easeFunctions;

class Easing {
public:
    static constexpr float Pi = 3.1415f;
    static constexpr float c1 = 1.70158;
    static constexpr float c2 = c1 * 1.525;
    static constexpr float c3 = c1 + 1;
    static constexpr float c4 = (2 * Pi) / 3;
    static constexpr float c5 = (2 * Pi) / 4.5;

    static inline float easeLinear(float x) {
        return x;
    }

    static inline float easeInQuad(float x) {
        return x * x;
    }

    static inline float easeOutQuad(float x) {
        return 1 - (1 - x) * (1 - x);
    }

    static inline float easeInOutQuad(float x) {
        return x < 0.5 ?
               2 * x * x :
               1 - std::pow(-2 * x + 2, 2.f) / 2;
    }

    static inline float easeInCubic(float x) {
        return x * x * x;
    }

    static inline float easeOutCubic(float x) {
        return 1 - std::pow(1 - x, 3.f);
    }

    static inline float easeInOutCubic(float x) {
        return x < 0.5 ?
               4 * x * x * x :
               1 - std::pow(-2 * x + 2, 3.f) / 2;
    }

    static inline float easeInQuart(float x) {
        return x * x * x * x;
    }

    static inline float easeOutQuart(float x) {
        return 1 - std::pow(1 - x, 4.f);
    }

    static inline float easeInOutQuart(float x) {
        return x < 0.5 ?
               8 * x * x * x * x :
               1 - std::pow(-2 * x + 2, 4.f) / 2;
    }

    static inline float easeInQuint(float x) {
        return x * x * x * x * x;
    }

    static inline float easeOutQuint(float x) {
        return 1 - std::pow(1 - x, 5.f);
    }

    static inline float easeInOutQuint(float x) {
        return x < 0.5 ?
               16 * x * x * x * x * x :
               1 - std::pow(-2 * x + 2, 5.f) / 2;
    }

    static inline float easeInSine(float x) {
        return 1 - cosf(x * Pi / 2);
    }

    static inline float easeOutSine(float x) {
        return sinf(x * Pi / 2);
    }

    static inline float easeInOutSine(float x) {
        return -(cosf(Pi * x) - 1) / 2;
    }

    static inline float easeInExpo(float x) {
        return x == 0 ? 0 : std::pow(2.f, 10 * x - 10);
    }

    static inline float easeOutExpo(float x) {
        return x == 1 ? 1 : 1 - std::pow(2.f, -10 * x);
    }

    static inline float easeInOutExpo(float x) {
        return x == 0 ? 0 : x == 1 ? 1 : x < 0.5 ?
                                         std::pow(2.f, 20 * x - 10) / 2 :
                                         (2 - std::pow(2.f, -20 * x + 10)) / 2;
    }

    static inline float easeInCirc(float x) {
        return 1 - std::sqrt(1 - std::pow(x, 2.f));
    }

    static inline float easeOutCirc(float x) {
        return std::sqrt(1 - std::pow(x - 1, 2.f));
    }

    static inline float easeInOutCirc(float x) {
        return x < 0.5 ?
               (1 - std::sqrt(1 - std::pow(2 * x, 2.f))) / 2 :
               (std::sqrt(1 - std::pow(-2 * x + 2, 2.f)) + 1) / 2;
    }

    static inline float easeInElastic(float x) {
        return x == 0 ? 0 : x == 1 ? 1 :
                            -std::pow(2.f, 10 * x - 10) * std::sin((x * 10 - 10.75f) * c4);
    }

    static inline float easeOutElastic(float x) {
        return x == 0 ? 0 : x == 1 ? 1 :
                                     std::pow(2.f, -10 * x) * std::sin((x * 10 - 0.75f) * c4) + 1;
    }

    static inline float easeInOutElastic(float x) {
        return x == 0 ? 0 : x == 1 ? 1 : x < 0.5 ?
                                             -(std::pow(2.f, 20 * x - 10) * std::sin((20 * x - 11.125f) * c5)) / 2 :
                                             std::pow(2.f, -20 * x + 10) * std::sin((20 * x - 11.125f) * c5) / 2 + 1;
    }

    static inline float easeInBack(float x) {
        return c3 * x * x * x - c1 * x * x;
    }

    static inline float easeOutBack(float x) {
        return 1 + c3 * std::pow(x - 1, 3.f) + c1 * std::pow(x - 1, 2.f);
    }

    static inline float easeInOutBack(float x) {
        return x < 0.5 ?
               (std::pow(2 * x, 2.f) * ((c2 + 1) * 2 * x - c2)) / 2 :
               (std::pow(2 * x - 2, 2.f) * ((c2 + 1) * (x * 2 - 2) + c2) + 2) / 2;
    }

    static float bounceOut(float x) {
        float n1 = 7.5625;
        float d1 = 2.75;
        if (x < 1 / d1) {
            return n1 * x * x;
        } else if (x < 2 / d1) {
            float x2 = x - 1.5f / d1;
            return n1 * x2 * x + .75f;
        } else if (x < 2.5 / d1) {
            float x2 = x - 2.25f / d1;
            return n1 * x2 * x + .9375f;
        } else {
            float x2 = x - 2.625f / d1;
            return n1 * x2 * x + .984375f;
        }
    }

    static inline float easeInBounce(float x) {
        return 1 - bounceOut(1 - x);
    }

    static inline float easeInOutBounce(float x) {
        return x < 0.5 ?
               (1 - bounceOut(1 - 2 * x)) / 2 :
               (1 + bounceOut(2 * x - 1)) / 2;
    }

    static ease_func_t getFuncByName(const std::string& name);

    static inline float apply(float value) {
        return easeInOutExpo(value);
    }

    static inline float apply(ease_index_t index, float value) {
        return easeFunctions[index](value);
    }
};