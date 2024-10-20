#pragma once

#if ESP32_CLI_ENABLE_TELNET

#include <WiFi.h>
#include "../Esp32Cli.h"

namespace Esp32Cli {
class TelnetServer {
public:
    explicit TelnetServer(std::shared_ptr<Cli> cli, uint16_t port = 23);

    ~TelnetServer();

private:
    [[noreturn]] void listen();

    static void startListen(void* arg) {
        static_cast<TelnetServer*>(arg)->listen();
    }

    static void handleClient(void* arg);

    std::shared_ptr<Cli> m_cli;
    WiFiServer m_wiFiServer{};
    TaskHandle_t m_listenTask{};
};
}

#endif
