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

#ifndef BSPATCH_H
# define BSPATCH_H

# include <stdint.h>

struct bspatch_stream {
    void* opaque;

    int (* read)(const struct bspatch_stream* stream, void* buffer, int length);
};

struct bspatch_stream_i {
    void* opaque;

    int (* read)(const struct bspatch_stream_i* stream, void* buffer, int pos, int length);
};

struct bspatch_stream_n {
    void* opaque;

    int (* write)(const struct bspatch_stream_n* stream, const void* buffer, int length);
};

int bspatch(struct bspatch_stream_i* oldstream, int64_t oldsize, struct bspatch_stream_n* newstream, int64_t newsize,
            struct bspatch_stream* patchstream);

#endif
