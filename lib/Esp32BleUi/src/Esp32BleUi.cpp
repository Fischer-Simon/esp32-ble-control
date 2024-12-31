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

#include "Esp32BleUi.h"
#include "Esp32BleUiClient.h"

#define BLE_SERVICE_UUID "afc3eba8-ba5e-42be-8d3c-94c7fe325ba1"
#define RX_CHARACTERISTIC_UUID "f72bac71-f66f-4cce-b83f-a4218f482706"
#define RX_ACK_CHARACTERISTIC_UUID "f72bac71-f66f-4cce-b83f-a4218f482708"
#define TX_CHARACTERISTIC_UUID "f72bac71-f66f-4cce-b83f-a4218f482707"
#define UI_CHARACTERISTIC_UUID "f72bac71-f66f-4cce-b83f-a4218f482709"

#define BLE_CHUNK_SIZE 222u

// #define VERBOSE_LOG

Esp32BleUi::Esp32BleUi(std::shared_ptr<Esp32Cli::Cli> cli)
    : m_cli(std::move(cli)) {
    NimBLEDevice::init(m_cli->getHostname());

    // TODO: Fix secure encrypted BLE for encrypted firmware updates.
#if 0
    NimBLEDevice::setSecurityAuth(true, true, true);
    NimBLEDevice::setSecurityPasskey(548613);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
#else
    NimBLEDevice::setSecurityAuth(false, false, true);
#endif

    m_bleServer = NimBLEDevice::createServer();
    m_bleServer->setCallbacks(this);

    m_bleService = m_bleServer->createService(BLE_SERVICE_UUID);

    m_rxCharacteristic = m_bleService->createCharacteristic(RX_CHARACTERISTIC_UUID, WRITE);
    m_rxAckCharacteristic = m_bleService->createCharacteristic(RX_ACK_CHARACTERISTIC_UUID, NOTIFY | READ);
    m_txCharacteristic = m_bleService->createCharacteristic(TX_CHARACTERISTIC_UUID, NOTIFY | READ);
    m_uiCharacteristic = m_bleService->createCharacteristic(UI_CHARACTERISTIC_UUID, NOTIFY | READ);

    m_rxCharacteristic->setCallbacks(this);

    m_bleService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(m_bleService->getUUID());
    pAdvertising->setMinInterval(100);
    pAdvertising->setMaxInterval(200);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    esp_bt_sleep_enable();

    if (xTaskCreate(&Esp32BleUi::runTxQueue, "tx_queue", 2048, this, 1, &m_txTask) != pdPASS) {
        log_e("Failed to create BLE Cli TX task");
        ESP.restart();
    }
}

Esp32BleUi::~Esp32BleUi() {
    if (m_metrics) {
        m_metrics->removeValueChangeCallback(this);
    }
    vTaskDelete(m_txTask);
    vTaskDelete(m_metricsTask);
    NimBLEDevice::deinit(true);
}

void Esp32BleUi::onWrite(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) {
    if (pCharacteristic == m_rxCharacteristic) {
        onRxWrite(desc);
    }
}

void Esp32BleUi::onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
    std::unique_lock<std::mutex> clientsLock{m_clientsMutex};
    m_clients.emplace_back(std::make_shared<Client>(m_cli, m_txTask, desc->conn_handle));
    auto client = m_clients.back();

    Serial.printf("Got new BLE client %i (%s)\n", desc->conn_handle,
                  NimBLEAddress(desc->peer_ota_addr).toString().c_str());

    if (m_bleServer->getConnectedCount() < NIMBLE_MAX_CONNECTIONS) {
        m_bleServer->startAdvertising();
    }

    pServer->setDataLen(desc->conn_handle, 0x00FB);

    pServer->updateConnParams(desc->conn_handle, 8, 24, 0, 400);

    auto clientPtr = new std::shared_ptr<Client>(client);
    if (xTaskCreatePinnedToCore(&runClient, "ble_client", 4096, clientPtr, 0, nullptr, ARDUINO_RUNNING_CORE) != pdPASS) {
        log_e("Failed to create BLE client task");
        delete clientPtr;
        m_clients.pop_back();
    }
}

void Esp32BleUi::onDisconnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
    std::unique_lock<std::mutex> clientsLock{m_clientsMutex};
    auto clientIterator = std::remove_if(
        m_clients.begin(), m_clients.end(),
        [desc](const std::shared_ptr<Client>& client) {
            return client->m_connHandle == desc->conn_handle;
        });
    if (clientIterator == m_clients.end()) {
        return;
    }
    const auto client = *clientIterator;
    std::unique_lock<std::mutex> clientRxBufferLock{client->m_rxBufferMutex};
    std::unique_lock<std::mutex> clientTxBufferLock{client->m_txBufferMutex};
    client->m_txTask = nullptr;
    client->m_connHandle = -1;
    client->m_txBufferNotifier.notify_all();
    client->m_rxBufferGivenNotifier.notify_all();
    client->m_rxBufferTakenNotifier.notify_all();
    client->onDisconnect();
    m_clients.erase(clientIterator);
    Serial.printf("Client %i disconnected\n", desc->conn_handle);
}

void Esp32BleUi::setKeyValueStore(std::shared_ptr<KeyValueStore> keyValueStore) {
    m_metrics = std::move(keyValueStore);
    if (xTaskCreatePinnedToCore(&Esp32BleUi::runMetrics, "metrics", 2048, this, 2, &m_metricsTask, ARDUINO_RUNNING_CORE) != pdPASS) {
        log_e("Failed to create BLE Cli metrics task");
        ESP.restart();
    }
    m_metrics->addValueChangeCallback(this, [this](const std::shared_ptr<const KeyValueStore::Value>& value) {
        std::unique_lock<std::mutex> lock{m_metricsMutex};
        m_changedMetrics.set(value->name(), value);
        xTaskNotifyGive(m_metricsTask);
    });
}

size_t Esp32BleUi::Client::write(const uint8_t* buffer, size_t size) {
    std::unique_lock<std::mutex> bufferLock{m_txBufferMutex};
    size_t bytesWritten = 0;
    do {
        if (!m_txTask) {
            return bytesWritten;
        }
        size_t writeSize = std::min<size_t>(size - bytesWritten, m_txBuffer.free());
        m_txBuffer.push(buffer + bytesWritten, writeSize);
        xTaskNotifyGive(m_txTask);
        bytesWritten += writeSize;
        if (bytesWritten == size) {
            break;
        }
#ifdef VERBOSE_LOG
        Serial.printf("Client %i waiting for TX buffer space (%i Bytes)\n", m_connHandle, size - bytesWritten);
#endif
        m_txBufferNotifier.wait(bufferLock);
    } while (true);
    return bytesWritten;
}

size_t Esp32BleUi::Client::write(uint8_t uint8) {
    std::unique_lock<std::mutex> bufferLock{m_txBufferMutex};
    while (m_txTask && m_txBuffer.free() < 1) {
#ifdef VERBOSE_LOG
        Serial.printf("Client %i waiting for TX buffer space (1 Byte)\n", m_connHandle);
#endif
        m_txBufferNotifier.wait(bufferLock);
    }
    if (!m_txTask) {
        return 0;
    }
    m_txBuffer.push(&uint8, 1);
    xTaskNotifyGive(m_txTask);
    return 1;
}

int Esp32BleUi::Client::availableForWrite() {
    std::unique_lock<std::mutex> bufferLock{m_txBufferMutex};
    return static_cast<int>(m_txBuffer.free());
}

int Esp32BleUi::Client::available() {
    std::unique_lock<std::mutex> bufferLock{m_rxBufferMutex};
    return static_cast<int>(m_rxBuffer.available());
}

size_t Esp32BleUi::Client::readBytes(char* buffer, size_t length) {
    std::unique_lock<std::mutex> bufferLock{m_rxBufferMutex};
    size_t bytesRead = 0;
    do {
        if (m_connHandle < 0) {
            return 0;
        }
        size_t readSize = std::min(length - bytesRead, m_rxBuffer.available());
        memcpy(buffer + bytesRead, m_rxBuffer.data(), readSize);
        m_rxBuffer.pop(readSize);
        bytesRead += readSize;
        m_rxBufferTakenNotifier.notify_all();
        if (bytesRead == length) {
            break;
        }
#ifdef VERBOSE_LOG
        Serial.printf("Client %i waiting for %i bytes\n", m_connHandle, length - readSize);
#endif
        m_rxBufferGivenNotifier.wait(bufferLock);
    } while (true);
#ifdef VERBOSE_LOG
    Serial.printf("Client %i got %i / %i bytes (%i Bytes left in buffer)\n", m_connHandle, bytesRead, length, m_rxBuffer.available());
#endif
    return bytesRead;
}

int Esp32BleUi::Client::read() {
    std::unique_lock<std::mutex> bufferLock{m_rxBufferMutex};
    int value = -1;
    while (m_connHandle >= 0 && !m_rxBuffer.available()) {
#ifdef VERBOSE_LOG
        Serial.printf("Client %i waiting for 1 byte\n", m_connHandle);
#endif
        m_rxBufferGivenNotifier.wait(bufferLock);
    }
    if (m_connHandle >= 0 && m_rxBuffer.available()) {
        value = m_rxBuffer.data()[0];
        m_rxBuffer.pop(1);
        m_rxBufferTakenNotifier.notify_all();
    }
#ifdef VERBOSE_LOG
    Serial.printf("Client %i got %s / one bytes (%i Bytes left in buffer)\n", m_connHandle, value >= 0 ? "1" : "0",
                  m_rxBuffer.available());
#endif
    return value;
}

int Esp32BleUi::Client::peek() {
    std::unique_lock<std::mutex> bufferLock{m_rxBufferMutex};
    if (m_rxBuffer.available()) {
        return m_rxBuffer.data()[0];
    }
    return -1;
}

void Esp32BleUi::Client::onCommandEnd() {
    std::unique_lock<std::mutex> bufferLock{m_txBufferMutex};
    while (m_txBuffer.shouldFlush()) {
        m_txBufferNotifier.wait(bufferLock);
    }
    m_txBuffer.markFlush();
    if (m_txTask) {
        xTaskNotifyGive(m_txTask);
    }
}

void Esp32BleUi::onRxWrite(ble_gap_conn_desc* desc) {
    std::shared_ptr<Client> client; {
        std::unique_lock<std::mutex> clientsLock{m_clientsMutex};
        auto clientIterator = std::find_if(
            m_clients.begin(), m_clients.end(),
            [desc](const std::shared_ptr<Client>& client) {
                return client->m_connHandle == desc->conn_handle;
            });
        if (clientIterator == m_clients.end()) {
            Serial.printf("Got %i bytes for unknown client\n", m_rxCharacteristic->getValue().size());
            return;
        }
        client = *clientIterator;
    }
    auto value = m_rxCharacteristic->getValue();

    std::unique_lock<std::mutex> clientBufferLock{client->m_rxBufferMutex};
    while (client->m_rxBuffer.free() < value.size()) {
#ifdef VERBOSE_LOG
        Serial.printf("Client %i waiting for buffer space (%i / %i Bytes available)\n", client->m_connHandle,
                      client->m_rxBuffer.free(), value.size());
#endif
        client->m_rxBufferTakenNotifier.wait(clientBufferLock);
    }
    client->m_rxBuffer.push(value.data(), value.size());
    client->m_rxAckPendingBytes += value.size();
    client->m_rxBufferGivenNotifier.notify_all();
#ifdef VERBOSE_LOG
    Serial.printf("Client %i received %i Bytes (%i Bytes in buffer)\n", client->m_connHandle,
                  value.size(), client->m_rxBuffer.available());
#endif
    if (m_txTask) {
        xTaskNotifyGive(m_txTask);
    }
}

[[noreturn]] void Esp32BleUi::processTxQueue() {
    bool dataToSend = false;
    os_mbuf* om;
    while (true) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(dataToSend ? 10 : 1000));
        dataToSend = false;

        // Check if attempting to send a BLE packet has a remote chance to succeed.
        om = ble_hs_mbuf_att_pkt();
        if (om == nullptr) {
            dataToSend = true;
            continue;
        }
        os_mbuf_free_chain(om);

        std::unique_lock<std::mutex> clientsLock{m_clientsMutex};
        for (auto& client: m_clients) {
            int rxAckSendRet = -999;
            int txRet = -999;
            std::unique_lock<std::mutex> clientBufferLock{client->m_txBufferMutex};
            if (client->m_rxAckPendingBytes > 0) {
                om = ble_hs_mbuf_from_flat(&client->m_rxAckPendingBytes, sizeof(client->m_rxAckPendingBytes));
                if (om == nullptr) {
                    dataToSend = true;
                    break;
                }
                rxAckSendRet = ble_gatts_notify_custom(client->m_connHandle, m_rxAckCharacteristic->getHandle(), om);
                if (rxAckSendRet == 0) {
#ifdef VERBOSE_LOG
                    Serial.printf("Sent RX ack (%i) to client %i\n", client->m_rxAckPendingBytes, client->m_connHandle);
#endif
                    client->m_rxAckPendingBytes = 0;
                } else {
                    dataToSend = true;
                    break;
                }
            }
            if (client->m_txBuffer.available() || client->m_txBuffer.shouldFlush()) {
                size_t sendSize = std::min(BLE_CHUNK_SIZE, client->m_txBuffer.available());
                om = ble_hs_mbuf_from_flat(client->m_txBuffer.data(), sendSize);
                if (om == nullptr) {
                    dataToSend = true;
                    break;
                }
                txRet = ble_gatts_notify_custom(client->m_connHandle, m_txCharacteristic->getHandle(), om);
                if (txRet == 0) {
                    client->m_txBuffer.pop(sendSize);
                    if (sendSize == 0) {
                        client->m_txBuffer.clearFlush();
                    }
                    client->m_txBufferNotifier.notify_all();
#ifdef VERBOSE_LOG
                    Serial.printf("Sent %i Bytes (%i left in TX buffer) to client %i\n", sendSize, client->m_txBuffer.available(), client->m_connHandle);
#endif
                }
            }
            dataToSend |= client->m_txBuffer.available() || client->m_txBuffer.shouldFlush() || client->m_rxAckPendingBytes;
            // Serial.printf("%i %i %i %i %i\n", client->m_txBuffer.available(), client->m_rxAckPendingBytes, rxAckSendRet, txRet, dataToSend);
        }
    }
}

void Esp32BleUi::processMetrics() {
    while (true) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000000));

        std::unique_lock<std::mutex> metricsLock{m_metricsMutex};

        auto it = m_changedMetrics.getEntries().begin();
        while (it != m_changedMetrics.getEntries().end()) {
            auto& metric = *it->second;
            size_t metricSize;
            size_t writtenSize = metric.writeToBuffer(
                m_changedMetricsBuffer.tail(), m_changedMetricsBuffer.free(), &metricSize
            );
            m_changedMetricsBuffer.push(writtenSize);
            if (metricSize > m_changedMetricsBuffer.capacity()) {
                log_e("Metric '%s' does not fit in buffer (%i > %i)\n", metric.name(), metricSize,
                      m_changedMetricsBuffer.capacity());
                ++it;
                continue;
            }
            if (writtenSize == 0) {
                processChangedMetricsBuffer();
            } else {
                ++it;
            }
        }
        processChangedMetricsBuffer();
        m_changedMetrics.clear();
    }
}

void Esp32BleUi::processChangedMetricsBuffer() {
    while (!m_changedMetricsBuffer.empty()) {
        size_t sendSize = std::min(BLE_CHUNK_SIZE, m_changedMetricsBuffer.available());
        bool sendFailure = false;
        std::unique_lock<std::mutex> clientsLock{m_clientsMutex};
        for (const auto& client: m_clients) {
            os_mbuf* om = ble_hs_mbuf_from_flat(m_changedMetricsBuffer.data(), sendSize);
            if (om == nullptr) {
                sendFailure = true;
                break;
            }
            if (ble_gatts_notify_custom(client->m_connHandle, m_uiCharacteristic->getHandle(), om) != 0) {
                sendFailure = true;
                break;
            }
        }
        if (!sendFailure) {
            m_changedMetricsBuffer.pop(sendSize);
        } else {
            delay(20);
        }
    }
}

void Esp32BleUi::runClient(void* arg) {
    const auto clientPtr = static_cast<std::shared_ptr<Client>*>(arg);
    auto client = *clientPtr;
    delete clientPtr;

    client->setTimeout(1000 * 60 * 60 * 24);
    client->executeCommandLine(Client::ExecType::Blocking);

    client.reset();
    vTaskDelete(nullptr);
}
