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

#include "Led/Animation.h"

#include <LedView.h>

#include "LedStrUtils.h"

namespace Led {

inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

Animation::RndDuration Animation::parseDuration(std::string durationStr) {
    if (durationStr.empty()) {
        return {};
    }

    bool delayIsGlobal = false;
    if (durationStr.front() == '=') {
        delayIsGlobal = true;
        durationStr.erase(durationStr.begin());
    }

    if (durationStr.size() > 2 && durationStr[0] == '[') {
        ltrim(durationStr, '[');
        rtrim(durationStr, ']');
        auto minMaxValue = split_str(durationStr, ',');
        if (minMaxValue.size() != 2) {
            return {};
        }
        auto minValue = parseDuration(minMaxValue[0]);
        auto maxValue = parseDuration(minMaxValue[1]);

        return {
            minValue.eval(1),
            maxValue.eval(1),
            minValue.perLed || maxValue.perLed,
            delayIsGlobal
        };
    }

    char* endOfNumber = nullptr;
    auto number = std::strtoul(durationStr.c_str(), &endOfNumber, 0);
    if (endOfNumber[0] == 's') {
        number *= 1000;
    }
    bool perLed = !ends_with(endOfNumber, "/n");
    return {durationFromMs(number), durationFromMs(number), perLed, delayIsGlobal};
}
}
