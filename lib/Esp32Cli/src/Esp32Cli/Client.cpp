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

#include "Esp32Cli/Client.h"

namespace Esp32Cli {
void Client::executeCommandLine(const ExecType execType) {
    do {
        if (execType == ExecType::NonBlocking && !available()) {
            return;
        }
        const int res = read();
        if (res < 0) {
            break;
        }
        const char c = static_cast<char>(res);
        if (m_parserState.isComment) {
            if (c == '\n') {
                m_parserState.isComment = false;
            }
            continue;
        }
        if (m_parserState.gotCommandEndOrNewLine) {
            m_parserState.gotCommandEndOrNewLine = false;
            if (c == '\r' || c == '\n') {
                continue;
            }
        }
        if (m_parserState.isEscapeSequence) {
            switch (c) {
                case 'n':
                    m_parserState.addCharToArg('\n');
                    break;
                case 't':
                    m_parserState.addCharToArg('\t');
                    break;
                case '\n':
                    // Ignore escaped new line to allow splitting a command over multiple lines.
                    m_parserState.gotCommandEndOrNewLine = true;
                    break;
                default:
                    m_parserState.addCharToArg(c);
                    break;
            }
            m_parserState.isEscapeSequence = false;
            continue;
        }
        if (c == '\\') {
            m_parserState.isEscapeSequence = true;
            continue;
        }
        if (m_parserState.isInQuotation && c == m_parserState.quotationCharacter) {
            m_parserState.isInQuotation = false;
            continue;
        }
        if (!m_parserState.isInQuotation && (c == '"' || c == '\'')) {
            m_parserState.isInQuotation = true;
            m_parserState.quotationCharacter = c;
            continue;
        }
        if (m_parserState.isInQuotation) {
            m_parserState.addCharToArg(c);
            continue;
        }

        if (c == ' ') {
            if (m_parserState.arg.empty()) {
                continue;
            }
            m_parserState.argv.emplace_back(m_parserState.arg);
            m_parserState.arg.clear();
            continue;
        }

        if (m_parserState.arg.empty() && c == '#') {
            m_parserState.isComment = true;
            continue;
        }

        if (c == ';' || c == '\n' || c == '\r') {
            if (!m_parserState.arg.empty()) {
                m_parserState.argv.emplace_back(m_parserState.arg);
                m_parserState.arg.clear();
            }
            m_cli->executeCommand(*this, m_parserState.argv);
            onCommandEnd();
            m_parserState.argv.clear();
            m_parserState.argvSize = 0;
            m_parserState.gotCommandEndOrNewLine = true;
            continue;
        }

        m_parserState.addCharToArg(c);
    } while (true);
}
}
