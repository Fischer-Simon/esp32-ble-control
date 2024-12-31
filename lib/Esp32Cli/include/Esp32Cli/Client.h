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

#include <list>

#include "Esp32Cli.h"

namespace Esp32Cli {
class Client : public Stream, public std::enable_shared_from_this<Client> {
public:
    enum class ExecType {
        Blocking,
        NonBlocking,
    };

    using OnDisconnectEventHandlerEntry = std::pair<void*, std::function<void()>>;

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
     * Line breaks also include carriage returns, all the following counts as a line break:
     * \\n, \\r, \\r\\n, \\n\\r.
     *
     * Lines starting with # will be ignored.
     *
     * @param execType Non-blocking returns immediately if no more data are available for read.
     */
    virtual void executeCommandLine(ExecType execType);

    void addDisconnectEventListener(void* owner, std::function<void()> callback) {
        m_onDisconnectEventHandlers.emplace_back(owner, std::move(callback));
    }

    void removeDisconnectEventListener(void* owner) {
        auto it = m_onDisconnectEventHandlers.begin();
        while (it != m_onDisconnectEventHandlers.end()) {
            if (it->first == owner) {
                it = m_onDisconnectEventHandlers.erase(it);
            } else {
                ++it;
            }
        }
    }

protected:
    /**
     * Callback executed when a command finished.
     */
    virtual void onCommandEnd() {
    }

    /**
    * Callback to execute by implementations when the client connection ends.
    */
    void onDisconnect() const {
        for (const auto& eventHandler : m_onDisconnectEventHandlers) {
            eventHandler.second();
        }
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
    std::list<OnDisconnectEventHandlerEntry> m_onDisconnectEventHandlers;
};
}
