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

#include "Esp32Cli/ScriptCommand.h"

#if ESP32_CLI_ENABLE_FS_COMMANDS

namespace Esp32Cli {
void ScriptCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) {
    if (argv.size() < 2) {
        Cli::printUsage(io, commandName, *this);
        return;
    }
    m_input = fopen(argv[1].c_str(), "r");
    if (!m_input) {
        io.printf("File %s not found\n", argv[1].c_str());
        return;
    }

    fseek(m_input, 0, SEEK_END);
    m_bytesLeft = ftell(m_input);
    fseek(m_input, 0, SEEK_SET);

    m_output = &io;
    executeCommandLine(ExecType::Blocking);
    m_output = nullptr;
    fclose(m_input);
}

void ScriptCommand::printUsage(Print& output) const {
    output.println("<file_name>");
}

void ScriptCommand::printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const {
    output.println("Run commands from a file");
}

size_t ScriptCommand::write(const uint8_t* buffer, size_t size) {
    return m_output->write(buffer, size);
}

size_t ScriptCommand::write(uint8_t c) {
    if (!m_output) {
        return 0;
    }
    m_output->write(c);
    return 1;
}

int ScriptCommand::available() {
    if (!m_input) {
        return 0;
    }
    return static_cast<int>(m_bytesLeft);
}

size_t ScriptCommand::readBytes(char* buffer, size_t length) {
    const size_t readSize = min(length, m_bytesLeft);
    fread(buffer, readSize, 1, m_input);
    m_bytesLeft -= readSize;
    return readSize;
}

int ScriptCommand::read() {
    const int result = fgetc(m_input);
    if (result > 0) {
        m_bytesLeft--;
    }
    return result;
}

int ScriptCommand::peek() {
    return -1; // TODO: Implement peek for file io.
}
}

#endif
