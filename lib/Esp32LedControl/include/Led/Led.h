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

#include "Position.h"

#include <NeoPixelBus.h>

namespace Led {
struct Led {
    using index_t = uint16_t;

    static constexpr index_t InvalidIndex = std::numeric_limits<index_t>::max();

    /**
     * The current color before any running animations have been applied.
     */
    RgbwColor currentBaseColor{0, 0, 0};

    /**
     * Position in space of this led relative to the @link LedString
     */
    Position position{0, 0, 0};
};
}
