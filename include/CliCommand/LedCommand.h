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

#include <Esp32Cli/Command.h>
#include <Esp32Cli/CommandGroup.h>

class LedManager;

namespace CliCommand {
class LedCommand : public Esp32Cli::Command {
public:
    explicit LedCommand(const std::shared_ptr<LedManager>& ledManager);

protected:
    std::shared_ptr<LedManager> m_ledManager;
};

class LsCommand : public LedCommand {
public:
    explicit LsCommand(const std::shared_ptr<LedManager>& ledManager) : LedCommand(ledManager) {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override;
};

class CatCommand : public LedCommand {
public:
    explicit CatCommand(const std::shared_ptr<LedManager>& ledManager) : LedCommand(ledManager) {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override ;

    void printUsage(Print& output) const override {
        output.println("<name>");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println("Print the LED strip content");
    }
};

class DebugCommand : public LedCommand {
public:
    explicit DebugCommand(const std::shared_ptr<LedManager>& ledManager) : LedCommand(ledManager) {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override ;

    void printUsage(Print& output) const override {
        output.println("<name>");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println("Print debug information about an LED strip");
    }
};

class GetPosCommand : public LedCommand {
public:
    explicit GetPosCommand(const std::shared_ptr<LedManager>& ledManager) : LedCommand(ledManager) {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override ;

    void printUsage(Print& output) const override {
        output.println("<name> <index>");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println("Print the 3D position of the given LED");
    }
};

class SetBrightnessCommand : public LedCommand {
public:
    explicit SetBrightnessCommand(const std::shared_ptr<LedManager>& ledManager) : LedCommand(ledManager) {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override;

    void printUsage(Print& output) const override {
        output.println("<name> <brightness>");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println("Set the brightness of an LED view");
    }
};

class SetPixelCommand : public LedCommand {
public:
    explicit SetPixelCommand(const std::shared_ptr<LedManager>& ledManager) : LedCommand(ledManager) {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override;

    void printUsage(Print& output) const override {
        output.println("<name> <index> <color>");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println("Set a single led pixel");
    }
};

class AnimatePixelCommand : public LedCommand {
public:
    explicit AnimatePixelCommand(const std::shared_ptr<LedManager>& ledManager) : LedCommand(ledManager) {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override;

    void printUsage(Print& output) const override {
        output.println("<name> <index> <delay> <duration> <color>");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println("Animate a single LED");
    }
};

class AnimateCommand : public LedCommand {
public:
    explicit AnimateCommand(const std::shared_ptr<LedManager>& ledManager) : LedCommand(ledManager) {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override;

    void printUsage(Print& output) const override {
        output.println("<name> <delay> <pixel_delay> <duration> <color> [<half_cycles> <blending> <easing>]");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println("Animate a string of LEDs");
    }
};

class Animate3DCommand : public LedCommand {
public:
    explicit Animate3DCommand(const std::shared_ptr<LedManager>& ledManager) : LedCommand(ledManager) {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override;

    void printUsage(Print& output) const override {
        output.println("<name> <delay> <pixel_delay> <duration> <x> <y> <z> <color>");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println("Animate a string of LEDs using 3D coordinates");
    }
};

class WriteCommand : public LedCommand {
public:
    explicit WriteCommand(const std::shared_ptr<LedManager>& ledManager) : LedCommand(ledManager) {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override;

    void printUsage(Print& output) const override {
        output.println("<name> [<offset>] <pixel_data>");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println("Write pixel data given as a hex string to the strip using an optional pixel offset");
    }
};

class ShowCommand : public LedCommand {
public:
    explicit ShowCommand(const std::shared_ptr<LedManager>& ledManager) : LedCommand(ledManager) {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) override;

    void printUsage(Print& output) const override {
        output.println("<name>");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println("Send the pixel data to the strip");
    }
};

class LedCommandGroup : public Esp32Cli::CommandGroup {
public:
    explicit LedCommandGroup(const std::shared_ptr<LedManager>& ledManager);
};
}
