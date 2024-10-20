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

#include "Easing.h"

#include <map>
#include <vector>

const std::array<ease_func_t, 30> easeFunctions = {
        &Easing::easeLinear,
        &Easing::easeInQuad,
        &Easing::easeOutQuad,
        &Easing::easeInOutQuad,
        &Easing::easeInCubic,
        &Easing::easeOutCubic,
        &Easing::easeInOutCubic,
        &Easing::easeInQuart,
        &Easing::easeOutQuart,
        &Easing::easeInOutQuart,
        &Easing::easeInQuint,
        &Easing::easeOutQuint,
        &Easing::easeInOutQuint,
        &Easing::easeInSine,
        &Easing::easeOutSine,
        &Easing::easeInOutSine,
        &Easing::easeInExpo,
        &Easing::easeOutExpo,
        &Easing::easeInOutExpo,
        &Easing::easeInCirc,
        &Easing::easeOutCirc,
        &Easing::easeInOutCirc,
        &Easing::easeInElastic,
        &Easing::easeOutElastic,
        &Easing::easeInOutElastic,
        &Easing::easeInBack,
        &Easing::easeOutBack,
        &Easing::easeInOutBack,
        &Easing::easeInBounce,
        &Easing::easeInOutBounce,
};

static const std::map<std::string, int> nameMap = {
        {"easeLinear", 0},
        {"easeInQuad", 1},
        {"easeOutQuad", 2},
        {"easeInOutQuad", 3},
        {"easeInCubic", 4},
        {"easeOutCubic", 5},
        {"easeInOutCubic", 6},
        {"easeInQuart", 7},
        {"easeOutQuart", 8},
        {"easeInOutQuart", 9},
        {"easeInQuint", 10},
        {"easeOutQuint", 11},
        {"easeInOutQuint", 12},
        {"easeInSine", 13},
        {"easeOutSine", 14},
        {"easeInOutSine", 15},
        {"easeInExpo", 16},
        {"easeOutExpo", 17},
        {"easeInOutExpo", 18},
        {"easeInCirc", 19},
        {"easeOutCirc", 20},
        {"easeInOutCirc", 21},
        {"easeInElastic", 22},
        {"easeOutElastic", 23},
        {"easeInOutElastic", 24},
        {"easeInBack", 25},
        {"easeOutBack", 26},
        {"easeInOutBack", 27},
        {"easeInBounce", 28},
        {"easeInOutBounce", 29},
};

ease_func_t Easing::getFuncByName(const std::string& name) {
    auto func = nameMap.find(name);
    if (func != nameMap.end()) {
        return easeFunctions[func->second];
    }
    return &Easing::easeLinear;
}
