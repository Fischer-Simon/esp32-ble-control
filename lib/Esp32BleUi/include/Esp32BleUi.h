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

#include "Esp32BleMetrics.h"

#include <esp_task_wdt.h>
#include <NimBLEDevice.h>
#include <mutex>
#include <condition_variable>
#include <Esp32Cli.h>

class Esp32BleUi : public NimBLECharacteristicCallbacks, public NimBLEServerCallbacks {
public:
    explicit Esp32BleUi(std::shared_ptr<Esp32Cli::Cli> cli);

    ~Esp32BleUi() override;

    void onWrite(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) override;

    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override;

    void onDisconnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override;

    void setMetrics(std::shared_ptr<Esp32BleMetrics> metrics);

private:
    class Client;

    void onRxWrite(ble_gap_conn_desc* desc);

    [[noreturn]] void processTxQueue();

    static void runTxQueue(void* arg) {
        static_cast<Esp32BleUi*>(arg)->processTxQueue();
    }

    [[noreturn]] void processMetrics();

    static void runMetrics(void* arg) {
        static_cast<Esp32BleUi*>(arg)->processMetrics();
    }

    static void runClient(void* arg);

    TaskHandle_t m_metricsTask{nullptr};
    TaskHandle_t m_txTask{nullptr};
    NimBLEServer* m_bleServer{nullptr};
    NimBLEService* m_bleService{nullptr};
    NimBLECharacteristic* m_rxCharacteristic;
    NimBLECharacteristic* m_rxAckCharacteristic;
    NimBLECharacteristic* m_txCharacteristic;
    NimBLECharacteristic* m_uiCharacteristic;
    std::mutex m_clientsMutex;
    std::vector<std::shared_ptr<Client> > m_clients;
    std::shared_ptr<Esp32Cli::Cli> m_cli;
    std::shared_ptr<Esp32BleMetrics> m_metrics;
};
