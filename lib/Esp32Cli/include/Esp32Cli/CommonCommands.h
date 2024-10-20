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
#include "CommandGroup.h"

namespace Esp32Cli {
class Cli;

class HostnameCommand : public Command {
public:
    explicit HostnameCommand(Cli& cli) : m_cli{cli} {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const override;

private:
    Cli& m_cli;
};

class MemCommand : public Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const override;
};

class ResetCommand : public Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const override;
};

#if ESP32_CLI_ENABLE_TELNET
class IpCommand : public Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override;
};
#endif

#if ESP32_CLI_ENABLE_FS_COMMANDS
class FsCommand : public CommandGroup {
public:
    FsCommand();
};
#endif

}
