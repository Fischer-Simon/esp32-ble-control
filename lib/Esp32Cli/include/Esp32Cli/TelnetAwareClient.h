#pragma once

#if ESP32_CLI_ENABLE_TELNET

#include <sstream>

#include "Client.h"

namespace Esp32Cli {
class TelnetAwareClient : public Client {
public:
    using ArduinoClient = ::Client;

    static constexpr int MaximumHistoryLength = 10;

    explicit TelnetAwareClient(const std::shared_ptr<Cli>& cli, std::unique_ptr<ArduinoClient> arduinoClient)
        : Client(cli),
          m_arduinoClient{std::move(arduinoClient)},
          m_lineBufferIterator{m_lineBuffer.end()},
          m_historyIterator{m_history.end()} {
    }

    void executeCommandLine(ExecType execType) override;

    size_t write(uint8_t) override;

    int available() override;

    int read() override;

    int peek() override;

private:
    void handleTelnetCommand();

    void overwriteLineBuffer(const std::string& line);

    std::unique_ptr<ArduinoClient> m_arduinoClient;
    bool m_gotTelnetCommand{false};
    size_t m_lineBufferReadPosition{0};
    std::string m_lineBuffer{};
    std::string::iterator m_lineBufferIterator{};
    std::vector<std::string> m_history{};
    std::vector<std::string>::iterator m_historyIterator{};
};
}

#endif
