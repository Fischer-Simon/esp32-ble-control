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

#include "CliCommand/LedCommand.h"

#include <Esp32Cli.h>
#include <LedManager.h>

#define LED_STRIP_FROM_FIRST_ARG m_ledManager->getLedViewByName(argv[1]);\
    if (ledStrip == nullptr) {\
        io.printf("Unknown LED view '%s'\n", argv[1].c_str());\
        return;\
    }\
    do {} while(0)


namespace CliCommand {
LedCommand::LedCommand(const std::shared_ptr<LedManager>& ledManager) : m_ledManager{ledManager} {
}

void LsCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) {
    for (const auto& led: m_ledManager->getLedViews()) {
        io.printf("%16s %16s  %s\n\r", led.first.c_str(), led.second->getType(), led.second->getName().c_str());
    }
}

void CatCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) {
    if (argv.size() != 2) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledStrip = LED_STRIP_FROM_FIRST_ARG;
    for (int i = 0; i < ledStrip->getLedCount(); i++) {
        auto pixel = ledStrip->getLedColor(i);
        io.printf("%i(%i,%i,%i) ", i, pixel.R, pixel.G, pixel.B);
    }
    io.println();
}

void DebugCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) {
    if (argv.size() != 2) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledStrip = LED_STRIP_FROM_FIRST_ARG;
}

void GetPosCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) {
    if (argv.size() != 3) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledStrip = LED_STRIP_FROM_FIRST_ARG;
    auto ledPosition = ledStrip->getLedPosition(strtoul(argv[2].c_str(), nullptr, 10));
    io.printf("Local: %i, %i, %i\nRelative: <TODO>\nGlobal: <TODO>\n", ledPosition.x, ledPosition.y, ledPosition.z);
}

void SetBrightnessCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) {
    if (argv.size() != 3) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledStrip = LED_STRIP_FROM_FIRST_ARG;
    float brightness = strtof(argv[2].c_str(), nullptr);
    ledStrip->setBrightness(brightness);
    LedView::AnimationConfig animation{
            ledStrip->getAnimationTargetColor(),
            std::chrono::milliseconds(0),
            std::chrono::milliseconds(0),
            std::chrono::milliseconds(150),
    };
    ledStrip->addAnimation(std::move(animation));
}

void SetPixelCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) {
    if (argv.size() != 4) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledStrip = LED_STRIP_FROM_FIRST_ARG;
    Led::Led::index_t ledIndex = strtoul(argv[2].c_str(), nullptr, 0);
    auto ledColor = m_ledManager->parseColor(argv[3], ledStrip);
    auto animation = LedView::AnimationConfig{
            ledColor,
            std::chrono::milliseconds(0),
            std::chrono::milliseconds(1)
    };
    animation.leds = {ledIndex};
    ledStrip->addAnimation(std::move(animation));
}

void AnimatePixelCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) {
    if (argv.size() != 6) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledStrip = LED_STRIP_FROM_FIRST_ARG;
    Led::Led::index_t ledIndex = strtoul(argv[2].c_str(), nullptr, 0);
    auto delay = std::chrono::milliseconds(strtoul(argv[3].c_str(), nullptr, 0));
    auto duration = std::chrono::milliseconds(strtol(argv[4].c_str(), nullptr, 0));
    auto ledColor = m_ledManager->parseColor(argv[5], ledStrip);
    auto animation = LedView::AnimationConfig{
            ledColor,
            delay,
            std::chrono::milliseconds(0),
            duration
    };
    animation.leds = {ledIndex};
    ledStrip->addAnimation(std::move(animation));
}

void AnimateCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) {
    if (argv.size() != 6 && argv.size() != 9) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledStrip = LED_STRIP_FROM_FIRST_ARG;
    auto delay = std::chrono::milliseconds(strtoul(argv[2].c_str(), nullptr, 0));
    auto ledDelay = std::chrono::milliseconds(strtol(argv[3].c_str(), nullptr, 0));
    auto duration = std::chrono::milliseconds(strtoul(argv[4].c_str(), nullptr, 0));
    auto ledColor = m_ledManager->parseColor(argv[5], ledStrip);

    if (argv.size() == 6) {
        ledStrip->addAnimation({
                                       ledColor,
                                       delay,
                                       ledDelay,
                                       duration,
                               });
        return;
    }

    uint8_t halfCycles = strtoul(argv[6].c_str(), nullptr, 0);
    const std::string& blendFunc = argv[7];
    const std::string& easeFunc = argv[8];

    Led::Blending blending = Led::Blending::Blend;
    if (blendFunc == "add") {
        blending = Led::Blending::Add;
    } else if (blendFunc == "overwrite") {
        blending = Led::Blending::Overwrite;
    }

    ledStrip->addAnimation({
                                   blending,
                                   Easing::getFuncByName(easeFunc),
                                   ledColor,
                                   delay,
                                   ledDelay,
                                   duration,
                                   halfCycles,
                           });
}

void Animate3DCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) {
    // TODO: 3D position aware LEDs
}

void WriteCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) {
}

void ShowCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) {
}

LedCommandGroup::LedCommandGroup(const std::shared_ptr<LedManager>& ledManager) {
    addSubCommand<LsCommand>("ls", ledManager);
    addSubCommand<CatCommand>("cat", ledManager);
    addSubCommand<DebugCommand>("debug", ledManager);
    addSubCommand<GetPosCommand>("get-pos", ledManager);
    addSubCommand<SetBrightnessCommand>("set-brightness", ledManager);
    addSubCommand<SetPixelCommand>("set-pixel", ledManager);
    addSubCommand<AnimatePixelCommand>("animate-pixel", ledManager);
    addSubCommand<AnimateCommand>("animate", ledManager);
    addSubCommand<Animate3DCommand>("animate-3d", ledManager);
    addSubCommand<WriteCommand>("write", ledManager);
    addSubCommand<ShowCommand>("show", ledManager);
}
}
