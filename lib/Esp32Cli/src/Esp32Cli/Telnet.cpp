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

#if ESP32_CLI_ENABLE_TELNET

#include "Esp32Cli/Telnet.h"
#include <Stream.h>
#include <cstdio>

void writeTelnetCommand(Telnet cmd, Stream& stream) {
    stream.write((uint8_t)Telnet::IAC);
    stream.write((uint8_t)cmd);
}

void writeTelnetOption(TelnetOption opt, Stream& stream) {
    stream.write((uint8_t)opt);
}

void writeTelnetResponse(Telnet cmd, TelnetOption opt, Stream& stream) {
    writeTelnetCommand(cmd, stream);
    writeTelnetOption(opt, stream);
}

const char* telnetCommandToString(Telnet command) {
    static char dummy[5];
    switch (command) {
        case Telnet::EOF_:
            return "EOF";
        case Telnet::SE:
            return "SE";
        case Telnet::NOP:
            return "NOP";
        case Telnet::DM:
            return "DM";
        case Telnet::BRK:
            return "BRK";
        case Telnet::IP:
            return "IP";
        case Telnet::AO:
            return "AO";
        case Telnet::AYT:
            return "AYT";
        case Telnet::EC:
            return "EC";
        case Telnet::EL:
            return "EL";
        case Telnet::GA:
            return "GA";
        case Telnet::SB:
            return "SB";
        case Telnet::WILL:
            return "WILL";
        case Telnet::WONT:
            return "WONT";
        case Telnet::DO:
            return "DO";
        case Telnet::DONT:
            return "DONT";
        case Telnet::IAC:
            return "IAC";
    }
    snprintf(dummy, 5, "0x%02x", (uint8_t)command);
    return dummy;
}

const char* telnetCommandToString(uint8_t c) {
    return telnetCommandToString((Telnet)c);
}

const char* telnetOptionToString(TelnetOption option) {
    static char dummy[5];
    switch (option) {
        case TelnetOption::Echo:
            return "Echo";
        case TelnetOption::SuppressGoAhead:
            return "SuppressGoAhead";
        case TelnetOption::Status:
            return "Status";
        case TelnetOption::TimingMark:
            return "TimingMark";
        case TelnetOption::TerminalType:
            return "TerminalType";
        case TelnetOption::WindowSize:
            return "WindowSize";
        case TelnetOption::TerminalSpeed:
            return "TerminalSpeed";
        case TelnetOption::RemoteFlowControl:
            return "RemoteFlowControl";
        case TelnetOption::Linemode:
            return "Linemode";
        case TelnetOption::EnvironmentVariables:
            return "EnvironmentVariables";
    }
    snprintf(dummy, 5, "0x%02x", (uint8_t)option);
    return dummy;
}

const char* telnetOptionToString(uint8_t c) {
    return telnetOptionToString((TelnetOption)c);
}

bool operator==(uint8_t v, Telnet c) {
    return v == (uint8_t)c;
}

#endif
