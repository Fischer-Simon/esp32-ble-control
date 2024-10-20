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

#include "RingBuffer.h"

#include <Arduino.h>
#include <map>
#include <string>
#include <mutex>

class Esp32BleMetrics {
public:
    void setMetricsTask(TaskHandle_t metricsTask) {
        m_metricsTask = metricsTask;
    }

    std::pair<RingBuffer<uint8_t, 512>&, std::unique_lock<std::mutex>> getChangedBuffer() {
        return {m_changedMetricsBuffer, std::move(std::unique_lock<std::mutex>(m_mutex))};
    }

protected:
    void notifyTask() {
        xTaskNotifyGive(m_metricsTask);
    }

    TaskHandle_t m_metricsTask{nullptr};
    std::mutex m_mutex;
    RingBuffer<uint8_t, 512> m_changedMetricsBuffer;
};
