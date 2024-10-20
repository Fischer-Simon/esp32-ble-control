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

#include "Esp32Cli/Command.h"

#include <Arduino.h>
#include <map>
#include <string>
#include <vector>
#include <memory>

namespace Esp32Cli {
class Cli : public std::enable_shared_from_this<Cli> {
    struct Private{ explicit Private() = default; };

public:
    explicit Cli(std::string hostname, Private);

    static std::shared_ptr<Cli> create(std::string hostname);

    void setFirmwareInfo(std::string firmwareInfo);

    template<
        typename CommandT,
        typename... Args
    >
    void addCommand(std::string name, Args&&... args) {
        addCommand(std::move(name), std::make_shared<CommandT>(std::forward<Args>(args)...));
    }

    void addCommand(std::string name, std::shared_ptr<Command> command);

    void executeCommand(Stream& io, std::vector<std::string>& argv) const;

    void printCommandNotFound(Print& output, const std::string& commandName) const;

    static void printUsage(Print& output, const std::string& commandName, const Command& command);

    void printWelcome(Print& output) const;

    void printPrompt(Print& output) const;

    const std::string& getHostname() const {
        return m_hostname;
    }

private:
    class HelpCommand : public Command {
    public:
        explicit HelpCommand(Cli& cli) : m_cli{cli} {
        }

        void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override;

    private:
        Cli& m_cli;
    };

    std::string m_hostname;
    std::string m_firmwareInfo{};
    std::map<std::string, std::shared_ptr<Command> > m_commands{};
};
}
