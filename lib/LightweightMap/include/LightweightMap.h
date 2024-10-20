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
#include <vector>
#include <cstring>

template<typename T>
class LightweightMap {
public:
    virtual ~LightweightMap() {
        for (auto& entry: m_entries) {
            free(entry.first);
            entry.first = nullptr;
        }
    }

    using entry_t = std::pair<char*, T>;

    void set(const char* key, T value) {
        auto it = std::lower_bound(
                m_entries.begin(), m_entries.end(), key,
                [](const entry_t& pair, const char* value) {
                    return std::strcmp(pair.first, value) < 0;
                }
        );
        if (it != m_entries.end() && std::strcmp(it->first, key) == 0) {
            it->second = std::move(value);
        } else {
            char* nameCopy = strdup(key);
            m_entries.insert(it, std::make_pair(nameCopy, std::move(value)));
        }
    }

    T& get(const char* key) {
        auto it = find(key);
        if (it == m_entries.end()) {
            set(key, std::move(T{}));
            return get(key);
        }

        return it->second;
    }

    const T& get(const char* key) const {
        auto it = find(key);
        if (it == m_entries.end()) {
            return m_emptyEntry;
        }

        return it->second;
    }

    typename std::vector<entry_t>::iterator find(const char* key) {
        auto it = std::lower_bound(
                m_entries.begin(), m_entries.end(), key,
                [](const entry_t& pair, const char* value) {
                    return std::strcmp(pair.first, value) < 0;
                }
        );
        if (it == m_entries.end() || std::strcmp(it->first, key) != 0) {
            return m_entries.end();
        }
        return it;
    }

    typename std::vector<entry_t>::const_iterator find(const char* key) const {
        auto it = std::lower_bound(
                m_entries.begin(), m_entries.end(), key,
                [](const entry_t& pair, const char* value) {
                    return std::strcmp(pair.first, value) < 0;
                }
        );
        if (it == m_entries.end() || std::strcmp(it->first, key) != 0) {
            return m_entries.end();
        }
        return it;
    }

    typename std::vector<entry_t>::const_iterator end() const {
        return m_entries.end();
    }

    const std::vector<entry_t>& getEntries() const {
        return m_entries;
    }

protected:
    T m_emptyEntry{};
    std::vector<entry_t> m_entries;
};
