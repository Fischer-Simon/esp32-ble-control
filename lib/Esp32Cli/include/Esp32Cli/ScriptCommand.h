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

#include "Client.h"
#include "Command.h"
#include "CommandGroup.h"

#if ESP32_CLI_ENABLE_FS_COMMANDS

namespace Esp32Cli {
class Cli;

class ScriptCommand : public Command, public Client {
public:
    explicit ScriptCommand(std::shared_ptr<Cli> cli) : Client{std::move(cli)} {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override;

    void printUsage(Print& output) const override;

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override;

    size_t write(const uint8_t* buffer, size_t size) override;

    size_t write(uint8_t) override;

    int available() override;

    size_t readBytes(char* buffer, size_t length) override;

    int read() override;

    int peek() override;

private:
    size_t m_bytesLeft{0};
    FILE* m_input{nullptr};
    Stream* m_output{nullptr};
};
}

#endif
