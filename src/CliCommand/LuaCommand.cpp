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

#include "CliCommand/LuaCommand.h"

#include <Esp32Cli.h>

namespace CliCommand {
class LuaCommand : public Esp32Cli::Command {
public:
    explicit LuaCommand(const std::shared_ptr<Lua>& lua) : m_lua{lua} {
    }

protected:
    std::shared_ptr<Lua> m_lua;
};

class LuaLoadCommand : public LuaCommand {
public:
    explicit LuaLoadCommand(const std::shared_ptr<Lua>& lua) : LuaCommand{lua} {
    }

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override {
        if (argv.size() != 2) {
            Esp32Cli::Cli::printUsage(io, commandName, *this);
            return;
        }
        m_lua->load(argv[1].c_str(), &io);
    }

    void printUsage(Print& output) const override {
        output.println("<file>");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println("Run lua code from a file");
    }
};

class LuaRunCommand : public LuaCommand {
public:
    explicit LuaRunCommand(const std::shared_ptr<Lua>& lua) : LuaCommand{lua} {
    }

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override {
        if (argv.size() != 2) {
            Esp32Cli::Cli::printUsage(io, commandName, *this);
            return;
        }
        m_lua->run(argv[1].c_str(), &io);
    }

    void printUsage(Print& output) const override {
        output.println("<lua_code>");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println("Run lua code from the command line");
    }
};

class LuaCallCommand : public LuaCommand {
public:
    explicit LuaCallCommand(const std::shared_ptr<Lua>& lua) : LuaCommand{lua} {
    }

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override {
        if (argv.size() < 2) {
            Esp32Cli::Cli::printUsage(io, commandName, *this);
            return;
        }

        if (argv.size() - 3 != argv[2].length()) {
            io.println("Number of arguments does not match.");
            Esp32Cli::Cli::printUsage(io, commandName, *this);
            return;
        }

        argv.erase(argv.begin());
        auto func = argv.front();
        argv.erase(argv.begin());
        auto argTypes = argv.front();
        argv.erase(argv.begin());

        std::vector<Lua::Arg> luaArgv;
        for (char argType: argTypes) {
            switch (argType) {
                case 'i':
                    luaArgv.emplace_back(static_cast<int>(strtol(argv.front().c_str(), nullptr, 0)));
                    break;
                case 'f':
                    luaArgv.emplace_back(strtof(argv.front().c_str(), nullptr));
                    break;
                case 's':
                    luaArgv.emplace_back(argv.front());
                    break;
                default:
                    io.printf("Unknown argument type %c\n", argType);
                    return;
            }
            argv.erase(argv.begin());
        }
        m_lua->call(func.c_str(), luaArgv, &io);
    }

    void printUsage(Print& output) const override {
        output.println("<function> <arg_types> <argv...>");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println(
            "Call a lua function.\narg_types is a string consisting of s(tring), i(nteger) and f(loat).\nSo for example sfii takes a string, a float and two integers.");
    }
};

LuaCommandGroup::LuaCommandGroup(const std::shared_ptr<Lua>& lua) {
    addSubCommand<LuaLoadCommand>("load", lua);
    addSubCommand<LuaRunCommand>("run", lua);
    addSubCommand<LuaCallCommand>("call", lua);
}
}
