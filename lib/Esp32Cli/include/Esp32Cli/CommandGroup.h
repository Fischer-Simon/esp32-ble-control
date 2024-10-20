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

#include <map>
#include <memory>

#include "Command.h"

namespace Esp32Cli {

class CommandGroup : public Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override {
        executeSubCommands(io, commandName, argv);
    }

    void printUsage(Print& output) const override {
        printSubCommandUsage(output);
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override;

protected:
    template<
        typename CommandT,
        typename... Args
    >
    void addSubCommand(std::string name, Args&&... args) {
        addSubCommand(std::move(name), std::make_shared<CommandT>(std::forward<Args>(args)...));
    }

    void addSubCommand(std::string name, std::shared_ptr<Command> command);

    void executeSubCommands(Stream& io, const std::string& commandName, std::vector<std::string>& argv);

    void printSubCommandUsage(Print& output) const;

    void printSubCommandHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const;

    std::map<std::string, std::shared_ptr<Command>> m_subCommands;
};

}
