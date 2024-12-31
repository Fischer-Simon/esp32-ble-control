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

#include <Esp32Cli/Client.h>

class Esp32BleUi::Client : public Esp32Cli::Client {
    friend class Esp32BleUi;

public:
    explicit Client(std::shared_ptr<Esp32Cli::Cli> cli, TaskHandle_t txTask, uint16_t connHandle)
        : Esp32Cli::Client{std::move(cli)}, m_txTask(txTask), m_connHandle(connHandle) {
    }

    size_t write(const uint8_t* buffer, size_t size) override;

    size_t write(uint8_t uint8) override;

    int availableForWrite() override;

    int available() override;

    size_t readBytes(char* buffer, size_t length) override;

    int read() override;

    int peek() override;

    void onCommandEnd() override;

private:
    TaskHandle_t m_txTask;
    uint16_t m_rxAckPendingBytes{0};
    RingBuffer<uint8_t, 1024> m_rxBuffer;
    std::mutex m_rxBufferMutex;
    std::condition_variable m_rxBufferGivenNotifier;
    std::condition_variable m_rxBufferTakenNotifier;
    RingBuffer<uint8_t, 512> m_txBuffer;
    std::mutex m_txBufferMutex;
    std::condition_variable m_txBufferNotifier;
    int32_t m_connHandle;
};
