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

#include <Arduino.h>
#include <memory>
#include <string>
#include <vector>

namespace Esp32Cli {

class Client;

class Command {
public:
    Command() = default;
    Command(const Command&) = delete;

    virtual ~Command() = default;

    /**
     * Execute the command.
     * @param io Command input and output
     * @param commandName Printable name for the command. Includes parent commands in case this is a sub command.
     * @param argv Command arguments. First entry is always the single name of the current command not including potential parent commands.
     * @param client Client this execution comes from. Can be null if the command was not executed by an external client.
     */
    virtual void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Client>& client) const = 0;

    virtual void printUsage(Print& output) const {
        output.println();
    }

    virtual void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const {
        printUsage(output);
    }
};
}
