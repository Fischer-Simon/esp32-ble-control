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

#if ESP32_CLI_ENABLE_TELNET

#include "Esp32Cli/TelnetServer.h"
#include "Esp32Cli/TelnetAwareClient.h"

namespace Esp32Cli {
TelnetServer::TelnetServer(std::shared_ptr<Cli> cli, const uint16_t port)
    : m_cli{std::move(cli)} {
    m_wiFiServer.begin(port, 1);
    m_wiFiServer.setNoDelay(true);
    if (xTaskCreate(&startListen, "telnet_server", 1024, this, 1, &m_listenTask) != pdPASS) {
        log_e("Failed to create telnet listen task");
        ESP.restart();
    }
}

TelnetServer::~TelnetServer() {
    m_wiFiServer.end();
    vTaskDelete(m_listenTask);
}

void TelnetServer::listen() {
    do {
        WiFiClient wiFiClient = m_wiFiServer.accept();
        if (wiFiClient) {
            std::unique_ptr<WiFiClient> wiFiClientPtr(new WiFiClient(wiFiClient));
            auto client = std::make_shared<TelnetAwareClient>(m_cli, std::move(wiFiClientPtr));
            auto clientPtr = new std::shared_ptr<TelnetAwareClient>(client);
            if (xTaskCreate(&handleClient, "telnet_client", 4096, clientPtr, 1, nullptr) != pdPASS) {
                log_e("Failed to create telnet client task");
                delete clientPtr;
            }
        } else {
            delay(20);
        }
    } while(true);
}

void TelnetServer::handleClient(void* arg) {
    const auto clientPtr = static_cast<std::shared_ptr<Client>*>(arg);
    auto client = *clientPtr;
    delete clientPtr;

    client->setTimeout(std::numeric_limits<unsigned long>::max());
    client->executeCommandLine(Client::ExecType::Blocking);

    client.reset();
    vTaskDelete(nullptr);
}
}

#endif
