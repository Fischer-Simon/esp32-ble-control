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

#ifndef HEATSHRINK_H
#define HEATSHRINK_H

#define HEATSHRINK_AUTHOR "Scott Vokes <vokes.s@gmail.com>"
#define HEATSHRINK_URL "https://github.com/atomicobject/heatshrink"

/* Version 0.4.1 */
#define HEATSHRINK_VERSION_MAJOR 0
#define HEATSHRINK_VERSION_MINOR 4
#define HEATSHRINK_VERSION_PATCH 1

#define HEATSHRINK_MIN_WINDOW_BITS 4
#define HEATSHRINK_MAX_WINDOW_BITS 15

#define HEATSHRINK_MIN_LOOKAHEAD_BITS 3

#define HEATSHRINK_LITERAL_MARKER 0x01
#define HEATSHRINK_BACKREF_MARKER 0x00

#endif
