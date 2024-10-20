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

#include "Esp32Cli.h"

namespace Esp32Cli {
class Client : public Stream {
public:
    enum class ExecType {
        Blocking,
        NonBlocking,
    };

    /**
     * Maximum size of the data in the argv array of the {@link ParserState}.
     */
    static constexpr size_t MaximumArgvSize = 2048;

    explicit Client(std::shared_ptr<Cli> cli) : m_cli{std::move(cli)} {
    }

    /**
     * Read available data and execute parsed commands.
     *
     * Multiple commands can be separated either via line breaks or semicolon.
     * Line breaks also include carriage returns, all of the following counts as a line break:
     * \\n, \\r, \\r\\n, \\n\\r.
     *
     * Lines starting with # will be ignored.
     *
     * @param execType Non blocking returns immediately if no more data are available for read.
     */
    virtual void executeCommandLine(ExecType execType);

protected:
    /**
     * Callback executed when a command finished.
     */
    virtual void onCommandEnd() {
    }

    std::shared_ptr<Cli> m_cli;

private:
    struct ParserState {
        std::string arg{};
        std::vector<std::string> argv{};
        size_t argvSize{0};
        bool isInQuotation{false};
        char quotationCharacter{0};
        bool isEscapeSequence{false};
        bool isComment{false};
        bool gotCommandEndOrNewLine{false};

        void addCharToArg(const char c) {
            if (argvSize > MaximumArgvSize) {
                return;
            }
            arg.push_back(c);
            argvSize++;
        }
    } m_parserState{};
};
}
