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

namespace Led {
struct Position {
    Position() : x{0}, y{0}, z{0} {
    }

    Position(int8_t x_, int8_t y_, int8_t z_) : x{x_}, y{y_}, z{z_} {
    }

    float distance(const Position& v) const {
        auto dx = static_cast<float>(x - v.x);
        auto dy = static_cast<float>(y - v.y);
        auto dz = static_cast<float>(z - v.z);

        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    int8_t x;
    int8_t y;
    int8_t z;
};
}
