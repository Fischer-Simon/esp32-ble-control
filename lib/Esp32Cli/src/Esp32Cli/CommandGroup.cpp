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

#include "Esp32Cli/CommandGroup.h"
#include "Esp32Cli.h"

namespace Esp32Cli {

void CommandGroup::printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const {
    if (argv.size() > 1) {
        printSubCommandHelp(output, commandName, argv);
        return;
    }

    Cli::printUsage(output, commandName, *this);
    for (const auto& subCommand: m_subCommands) {
        Cli::printUsage(output, commandName + " " + subCommand.first, *subCommand.second);
    }
}

void CommandGroup::addSubCommand(std::string name, std::shared_ptr<Command> command) {
    m_subCommands.emplace(std::move(name), std::move(command));
}

void CommandGroup::executeSubCommands(Stream& io, const std::string& commandName, std::vector<std::string>& argv) {
    if (argv.size() < 2) {
        Cli::printUsage(io, commandName, *this);
        return;
    }

    const auto command = m_subCommands.find(argv[1]);
    if (command == m_subCommands.end()) {
        Cli::printUsage(io, commandName, *this);
        return;
    }

    argv.erase(argv.begin());
    command->second->execute(io, commandName + " " + argv[0], argv);
}

void CommandGroup::printSubCommandUsage(Print& output) const {
    output.print('[');
    bool first = true;
    for (const auto& subCommand : m_subCommands) {
        if (first) {
            first = false;
        } else {
            output.print(',');
        }
        output.print(subCommand.first.c_str());
    }
    output.println("] <...>");
}

void CommandGroup::printSubCommandHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const {
    if (argv.size() < 2) {
        return;
    }

    const std::string mainCommandName = argv[0];
    const std::string subCommandName = argv[1];
    const auto command = m_subCommands.find(subCommandName);
    if (command == m_subCommands.end()) {
        output.printf("%s: %s: no such sub command\n", argv[0].c_str(), argv[1].c_str());
        return;
    }

    argv.erase(argv.begin());
    command->second->printHelp(output, mainCommandName + " " + subCommandName, argv);
}

}