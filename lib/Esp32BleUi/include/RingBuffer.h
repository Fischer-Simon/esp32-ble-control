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

#include <cstdint>
#include <cstring>

template<typename T, size_t Size>
class RingBuffer {
public:
    RingBuffer() = default;

    RingBuffer(const RingBuffer&) = delete;

    bool push(const T* val, size_t size) {
        if (m_bufferFill + size > Size) {
            return false;
        }
        memcpy(m_buffer + m_bufferFill, val, size);
        m_bufferFill += size;
        return true;
    }

    bool push(size_t size) {
        if (m_bufferFill + size > Size) {
            return false;
        }
        m_bufferFill += size;
        return true;
    }

    bool pop(size_t amount) {
        if (m_bufferFill < amount) {
            return false;
        }
        memmove(m_buffer, m_buffer + amount, m_bufferFill - amount);
        m_bufferFill -= amount;
        return true;
    }

    const T* data() {
        return m_buffer;
    }

    T* tail() {
        return m_buffer + m_bufferFill;
    }

    size_t available() const {
        return m_bufferFill;
    }

    bool empty() const {
        return m_bufferFill == 0;
    }

    size_t free() const {
        if (m_shouldFlush) {
            return 0;
        }
        return Size - m_bufferFill;
    }

    void clear() {
        m_bufferFill = 0;
        m_shouldFlush = false;
    }

    bool shouldFlush() const {
        return m_shouldFlush;
    }

    void markFlush() {
        m_shouldFlush = true;
    }

    void clearFlush() {
        m_shouldFlush = false;
    }

    static constexpr size_t capacity() {
        return Size;
    }

private:
    T m_buffer[Size]{0};
    volatile size_t m_bufferFill{0};
    bool m_shouldFlush{false};
};
