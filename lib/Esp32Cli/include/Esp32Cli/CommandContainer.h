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

#include "Command.h"

#include <LightweightMap.h>

#include <memory>
#include <vector>

namespace Esp32Cli {
class CommandContainer {
public:
    virtual ~CommandContainer() = default;

    template<
            typename CommandT,
            typename... Args
    >
    void addCommand(const char* name, Args&& ... args) {
        addCommand(name, std::unique_ptr<CommandT>(new CommandT(std::forward<Args>(args)...)));
    }

    void addCommand(const char* name, std::unique_ptr<Command> command) {
        m_commands.set(name, std::move(command));
    }

protected:
    const Command* getCommand(const char* name) const {
        auto it = m_commands.find(name);
        if (it != m_commands.end()) {
            return it->second.get();
        }
    }

    LightweightMap<std::unique_ptr<Command>> m_commands;
};
}
