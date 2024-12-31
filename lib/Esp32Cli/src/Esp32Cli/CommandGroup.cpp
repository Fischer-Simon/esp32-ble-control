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
        const std::string mainCommandName = argv[0];
        const std::string subCommandName = argv[1];
        const Command* command = getCommand(subCommandName.c_str());
        if (command == nullptr) {
            output.printf("%s: %s: no such sub command\n", commandName.c_str(), subCommandName.c_str());
            return;
        }

        argv.erase(argv.begin());
        command->printHelp(output, mainCommandName + " " + subCommandName, argv);
        return;
    }

    Cli::printUsage(output, commandName, *this);
    for (const auto& command: m_commands.getEntries()) {
        Cli::printUsage(output, commandName + " " + command.first, *command.second);
    }
}

void CommandGroup::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Client>& client) const {
    if (argv.size() < 2) {
        Cli::printUsage(io, commandName, *this);
        return;
    }

    const Command* command = getCommand(argv[1].c_str());
    if (command == nullptr) {
        Cli::printUsage(io, commandName, *this);
        return;
    }

    argv.erase(argv.begin());
    command->execute(io, commandName + " " + argv[0], argv, client);
}

void CommandGroup::printUsage(Print& output) const {
    output.print('[');
    bool first = true;
    for (const auto& command : m_commands.getEntries()) {
        if (first) {
            first = false;
        } else {
            output.print(',');
        }
        output.print(command.first);
    }
    output.println("] <...>");
}

}