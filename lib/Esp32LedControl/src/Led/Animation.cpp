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

namespace Led {
Animation::duration Animation::parseDuration(const std::string& durationStr) {
    char* endOfNumber = nullptr;
    auto number = std::strtoul(durationStr.c_str(), &endOfNumber, 0);
    if (endOfNumber[0] == 's') {
        number *= 1000;
    } else if (std::string{endOfNumber} == "min") {
        number *= 60000;
    }
    return std::chrono::milliseconds(number);
}
}
