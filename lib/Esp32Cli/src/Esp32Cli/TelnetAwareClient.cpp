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

#include "Esp32Cli/TelnetAwareClient.h"
#include "Esp32Cli/Telnet.h"
#include "Esp32Cli/Ansi.h"

namespace Esp32Cli {
void TelnetAwareClient::executeCommandLine(const ExecType execType) {
    char c;

    do {
        if (execType == ExecType::NonBlocking && !m_arduinoClient->available()) {
            return;
        }
        while (m_arduinoClient->readBytes(&c, 1) != 1) {
            if (execType == ExecType::NonBlocking) {
                return;
            }
        }

        if (c == Telnet::IAC) {
            handleTelnetCommand();
            continue;
        }

        if (c == '\n' || c == '\r') {
            if (m_gotTelnetCommand) {
                // Read the additional null byte sent by the telnet client.
                m_arduinoClient->read();
            }

            if (!m_lineBuffer.empty() || m_historyIterator != m_history.end() || c == '\n') {
                if (m_historyIterator != m_history.end()) {
                    m_lineBuffer = *m_historyIterator;
                }
                if (m_history.empty() || m_history.back() != m_lineBuffer) {
                    m_history.push_back(m_lineBuffer);
                }
                if (m_history.size() > MaximumHistoryLength) {
                    m_history.erase(m_history.begin());
                }
                m_historyIterator = m_history.end();

                m_arduinoClient->println();
                m_lineBufferReadPosition = 0;
                m_lineBuffer.push_back('\n');
                Client::executeCommandLine(ExecType::Blocking);
                overwriteLineBuffer("");
                m_cli->printPrompt(*this);
            }
        } else if (c == 3) {
            // CTRL+c
            m_arduinoClient->print("^C\r\n");
            m_cli->printPrompt(*m_arduinoClient);
            overwriteLineBuffer("");
            m_historyIterator = m_history.end();
        } else if (c == 27) {
            // ESC
            m_arduinoClient->readBytes(&c, 1);
            if (c != '[') {
                continue;
            }
            m_arduinoClient->readBytes(&c, 1);
            switch (c) {
                case 'A': // Up
                    if (m_historyIterator != m_history.begin()) {
                        const std::string& currentLine = m_historyIterator == m_history.end()
                                                             ? m_lineBuffer
                                                             : *m_historyIterator;
                        --m_historyIterator;
                        const std::string& newLine = *m_historyIterator;
                        m_arduinoClient->print(ANSI_CLEAR_LINE"\r");
                        m_cli->printPrompt(*m_arduinoClient);
                        m_arduinoClient->print(newLine.c_str());
                    }
                    break;
                case 'B': // Down
                    if (m_historyIterator != m_history.end()) {
                        const std::string& currentLine = *m_historyIterator;
                        ++m_historyIterator;
                        const std::string& newLine = m_historyIterator == m_history.end()
                                                         ? m_lineBuffer
                                                         : *m_historyIterator;
                        m_arduinoClient->print(ANSI_CLEAR_LINE"\r");
                        m_cli->printPrompt(*m_arduinoClient);
                        m_arduinoClient->print(newLine.c_str());
                    }
                    break;
                case 'C': // Right
                    if (m_lineBufferIterator != m_lineBuffer.end()) {
                        ++m_lineBufferIterator;
                        m_arduinoClient->print(ANSI_CURSOR_RIGHT(1));
                    }
                    break;
                case 'D': // Left
                    if (m_historyIterator != m_history.end()) {
                        overwriteLineBuffer(*m_historyIterator);
                        m_historyIterator = m_history.end();
                    }
                    if (m_lineBufferIterator != m_lineBuffer.begin()) {
                        --m_lineBufferIterator;
                        m_arduinoClient->print(ANSI_CURSOR_LEFT(1));
                    }
                    break;
                default: // Unhandled unknown
                    break;
            }
        } else if (c == 4) {
            // EOF
            m_arduinoClient->stop();
        } else if (c == 127 || c == 8) {
            // DEL
            if (m_historyIterator != m_history.end()) {
                overwriteLineBuffer(*m_historyIterator);
                m_historyIterator = m_history.end();
            }
            if (!m_lineBuffer.empty() && m_lineBufferIterator != m_lineBuffer.begin()) {
                m_lineBufferIterator = m_lineBuffer.erase(m_lineBufferIterator - 1);
                int distance = m_lineBufferIterator - m_lineBuffer.begin();
                m_arduinoClient->printf(ANSI_CURSOR_LEFT(1) ANSI_CURSOR_SAVE "%s " ANSI_CURSOR_RESTORE,
                                 m_lineBuffer.c_str() + distance);
            }
        } else if (c >= 32) {
            if (m_historyIterator != m_history.end()) {
                overwriteLineBuffer(*m_historyIterator);
                m_historyIterator = m_history.end();
            }
            const int distance = m_lineBufferIterator - m_lineBuffer.begin();
            if (m_lineBuffer.size() < MaximumArgvSize) {
                m_lineBufferIterator = m_lineBuffer.insert(m_lineBufferIterator, (char) c) + 1;
            }
            if (m_lineBufferIterator == m_lineBuffer.end()) {
                if (m_gotTelnetCommand) {
                    m_arduinoClient->write(c);
                }
            } else {
                m_arduinoClient->printf(ANSI_CURSOR_SAVE "%s" ANSI_CURSOR_RESTORE ANSI_CURSOR_RIGHT(1),
                                 m_lineBuffer.c_str() + distance);
            }
        }
    } while (true);
}

size_t TelnetAwareClient::write(uint8_t value) {
    if (value == '\n') {
        m_arduinoClient->write('\r');
    }
    return m_arduinoClient->write(value);
}

int TelnetAwareClient::available() {
    return static_cast<int>(m_lineBuffer.length() - m_lineBufferReadPosition);
}

int TelnetAwareClient::read() {
    int res = -1;
    if (m_lineBufferReadPosition < m_lineBuffer.length()) {
        res = m_lineBuffer[m_lineBufferReadPosition];
        m_lineBufferReadPosition++;
    }
    return res;
}

int TelnetAwareClient::peek() {
    int res = -1;
    if (m_lineBufferReadPosition < m_lineBuffer.length()) {
        res = m_lineBuffer[m_lineBufferReadPosition];
    }
    return res;
}

void TelnetAwareClient::handleTelnetCommand() {
    union {
        uint8_t c{};
        Telnet cmd;
    };
    uint8_t buf[17];
    buf[16] = 0;

    if (!m_gotTelnetCommand) {
        m_gotTelnetCommand = true;
        writeTelnetResponse(Telnet::WILL, TelnetOption::Echo, *m_arduinoClient);
        writeTelnetResponse(Telnet::DONT, TelnetOption::Echo, *m_arduinoClient);
        writeTelnetResponse(Telnet::WILL, TelnetOption::SuppressGoAhead, *m_arduinoClient);
        m_cli->printWelcome(*m_arduinoClient);
    }

    m_arduinoClient->readBytes(&c, 1);

    TelnetOption opt;
    if (cmd == Telnet::WILL || cmd == Telnet::DO || cmd == Telnet::WONT || cmd == Telnet::DONT || cmd == Telnet::SB) {
        m_arduinoClient->readBytes(reinterpret_cast<uint8_t*>(&opt), 1);
    }

    switch (cmd) {
        case Telnet::EOF_:
            m_arduinoClient->stop();
            return;
        /*
        case Telnet::DO:
            switch (opt) {
                case TelnetOption::SuppressGoAhead:
                    writeTelnetResponse(TelnetCommand::WILL, opt, m_client);
                    break;
                default:
                    writeTelnetResponse(TelnetCommand::WONT, opt, m_client);
                    break;
            }
            break;
        */
        /*
        case Telnet::WILL:
            switch (opt) {
                case TelnetOption::TerminalType:
                    writeTelnetResponse(TelnetCommand::DO, opt, m_client);
                    writeTelnetResponse(TelnetCommand::SB, opt, m_client);
                    m_arduinoClient->write((char)1);
                    writeTelnetCommand(TelnetCommand::SE, m_client);
                    break;
                default:
                    break;
            }
            break;
        */
        /*
        case Telnet::WONT:
            writeTelnetCommand(TelnetCommand::DONT, m_client);
            writeTelnetOption(opt, m_client);
            break;
        */
        /*
        case Telnet::DONT:
            writeTelnetCommand(TelnetCommand::WONT, m_client);
            writeTelnetOption(opt, m_client);
            break;
        */
        case Telnet::SB:
            m_arduinoClient->readBytes(&c, 1);
            for (int i = 0; cmd != Telnet::IAC; i++, m_arduinoClient->readBytes(&c, 1)) {
                if (i >= 16) {
                    continue;
                }
                buf[i] = c;
            }
            /*
            m_arduinoClient->printf(",%i", buf[0]);
            if (buf[0] == 1) {
                // Value requested
            } else {
                // Value provided
                m_arduinoClient->printf(",%s", &buf[1]);
            }
            */
            m_arduinoClient->readBytes(&c, 1); // Should be SE.
        default: // Unhandled
            break;
    }
}

void TelnetAwareClient::overwriteLineBuffer(const std::string& line) {
    m_lineBuffer = line;
    m_lineBufferIterator = m_lineBuffer.end();
}

}

#endif
