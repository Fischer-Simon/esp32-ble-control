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

#include "Esp32Cli.h"

#include "Esp32Cli/CommonCommands.h"
#include "Esp32Cli/ScriptCommand.h"

namespace Esp32Cli {
Cli::Cli(std::string hostname, Cli::Private) : m_hostname{std::move(hostname)} {
    addCommand<HelpCommand>("help", *this);
    addCommand<HostnameCommand>("hostname", *this);
    addCommand<MemCommand>("mem");
    addCommand<ResetCommand>("reset");
#if ESP32_CLI_ENABLE_TELNET
    addCommand<IpCommand>("ip");
#endif
#if ESP32_CLI_ENABLE_FS_COMMANDS
    addCommand<FsCommand>("fs");
#endif
}

std::shared_ptr<Cli> Cli::create(std::string hostname) {
    const auto cli = std::make_shared<Cli>(std::move(hostname), Private{});
#if ESP32_CLI_ENABLE_FS_COMMANDS
    // Add the script command here instead of the constructor because shared_from_this() is not available in the
    // constructor and leads to a crash.
    cli->addCommand<ScriptCommand>("script", cli->shared_from_this());
#endif
    return cli;
}

void Cli::setFirmwareInfo(std::string firmwareInfo) {
    m_firmwareInfo = std::move(firmwareInfo);
}

void Cli::executeCommand(Stream& io, std::vector<std::string>& argv) const {
    if (argv.empty()) {
        return;
    }

    const std::string& commandName = argv.front();

    auto command = getCommand(commandName.c_str());
    if (command == nullptr) {
        printCommandNotFound(io, commandName);
        return;
    }
    command->execute(io, commandName, argv);
}

void Cli::printCommandNotFound(Print& output, const std::string& commandName) const {
    output.printf("%s: %s: command not found\n", m_hostname.c_str(), commandName.c_str());
}

void Cli::printUsage(Print& output, const std::string& commandName, const Command& command) {
    output.print("Usage: ");
    output.print(commandName.c_str());
    output.print(' ');
    command.printUsage(output);
}

void Cli::printWelcome(Print& output) const {
    output.print("Welcome to the ");
    output.print(m_hostname.c_str());
    output.println(" command line interface.");
    if (!m_firmwareInfo.empty()) {
        output.println(m_firmwareInfo.c_str());
    }
    output.println("You can type your commands, type 'help' for a list of commands");
    printPrompt(output);
}

void Cli::printPrompt(Print& output) const {
    output.print(m_hostname.c_str());
    output.print("> ");
}

void Cli::HelpCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const {
    if (argv.size() <= 1) {
        io.print("Available commands: ");
        bool isFirst = true;
        for (auto& command: m_cli.m_commands.getEntries()) {
            if (isFirst) {
                isFirst = false;
            } else {
                io.print(", ");
            }
            io.print(command.first);
        }
        io.println();
        io.println("Type help <command> for more information.");
        return;
    }

    argv.erase(argv.begin());
    const auto command = m_cli.getCommand(argv[0].c_str());
    if (command != nullptr) {
        command->printHelp(io, argv[0], argv);
    } else {
        io.printf("help: %s: no such command\n", argv[0].c_str());
    }
}
}
