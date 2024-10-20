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

void LsCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const {
    for (const auto& led: m_ledManager->getLedViews()) {
        io.printf("%16s %16s  %s\n\r", led.first, led.second->getType(), led.second->getName().c_str());
    }
}

void CatCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const {
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

void DebugCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const {
    if (argv.size() != 2) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledStrip = LED_STRIP_FROM_FIRST_ARG;
}

void GetPosCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const {
    if (argv.size() != 3) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledStrip = LED_STRIP_FROM_FIRST_ARG;
    auto ledPosition = ledStrip->getLedPosition(strtoul(argv[2].c_str(), nullptr, 10));
    io.printf("Local: %i, %i, %i\nRelative: <TODO>\nGlobal: <TODO>\n", ledPosition.x, ledPosition.y, ledPosition.z);
}

void SetBrightnessCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const {
    if (argv.size() != 3) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledStrip = LED_STRIP_FROM_FIRST_ARG;
    float brightness = strtof(argv[2].c_str(), nullptr);
    ledStrip->setBrightness(brightness);
    std::unique_ptr<AnimationConfig> animation{new AnimationConfig{
            ledStrip->getAnimationTargetColor(),
            AnimationConfig::LedDelayMs(0),
            AnimationConfig::LedDurationMs(150),
    }};
    ledStrip->addAnimation(std::move(animation));
}

void SetPixelCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const {
    if (argv.size() != 4) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledStrip = LED_STRIP_FROM_FIRST_ARG;
    Led::Led::index_t ledIndex = strtoul(argv[2].c_str(), nullptr, 0);
    auto ledColor = m_ledManager->parseColor(argv[3], ledStrip);
    std::unique_ptr<AnimationConfig> animation{new AnimationConfig{
            ledColor,
            AnimationConfig::LedDelayMs(0),
            AnimationConfig::LedDurationMs(1),
    }};
    animation->leds = {ledIndex};
    ledStrip->addAnimation(std::move(animation));
}

void AnimatePixelCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const {
    if (argv.size() != 6) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledStrip = LED_STRIP_FROM_FIRST_ARG;
    Led::Led::index_t ledIndex = strtoul(argv[2].c_str(), nullptr, 0);
    auto delay = std::chrono::milliseconds(strtoul(argv[3].c_str(), nullptr, 0));
    auto duration = std::chrono::milliseconds(strtol(argv[4].c_str(), nullptr, 0));
    auto ledColor = m_ledManager->parseColor(argv[5], ledStrip);
    std::unique_ptr<AnimationConfig> animation{new AnimationConfig{
            ledColor,
            delay,
            AnimationConfig::LedDelayMs(0),
            duration
    }};
    animation->leds = {ledIndex};
    ledStrip->addAnimation(std::move(animation));
}

void AnimateCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const {
    if (argv.size() != 6 && argv.size() != 9) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledStrip = LED_STRIP_FROM_FIRST_ARG;
    auto delay = std::chrono::milliseconds(strtoul(argv[2].c_str(), nullptr, 0));
    auto ledDelay = std::chrono::milliseconds(strtol(argv[3].c_str(), nullptr, 0));
    auto duration = std::chrono::milliseconds(strtoul(argv[4].c_str(), nullptr, 0));
    auto ledColor = m_ledManager->parseColor(argv[5], ledStrip);

    std::unique_ptr<AnimationConfig> animation;
    if (argv.size() == 6) {
        animation.reset(new AnimationConfig{
                ledColor,
                delay,
                ledDelay,
                duration,
        });
    } else {
        uint8_t halfCycles = strtoul(argv[6].c_str(), nullptr, 0);
        const std::string& blendFunc = argv[7];
        const std::string& easeFunc = argv[8];

        Led::Blending blending = Led::Blending::Blend;
        if (blendFunc == "add") {
            blending = Led::Blending::Add;
        } else if (blendFunc == "overwrite") {
            blending = Led::Blending::Overwrite;
        }

        animation.reset(new AnimationConfig{
                blending,
                Easing::getFuncByName(easeFunc),
                ledColor,
                delay,
                ledDelay,
                duration,
                halfCycles,
        });
    }

    ledStrip->addAnimation(std::move(animation));
}

void Animate3DCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const {
    // TODO: 3D position aware LEDs
}

void WriteCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const {
}

void ShowCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const {
}

LedCommandGroup::LedCommandGroup(
        const std::shared_ptr<LedManager>& ledManager) {
    addCommand<LsCommand>("ls", ledManager);
    addCommand<CatCommand>("cat", ledManager);
    addCommand<DebugCommand>("debug", ledManager);
    addCommand<GetPosCommand>("get-pos", ledManager);
    addCommand<SetBrightnessCommand>("set-brightness", ledManager);
    addCommand<SetPixelCommand>("set-pixel", ledManager);
    addCommand<AnimatePixelCommand>("animate-pixel", ledManager);
    addCommand<AnimateCommand>("animate", ledManager);
    addCommand<Animate3DCommand>("animate-3d", ledManager);
    addCommand<WriteCommand>("write", ledManager);
    addCommand<ShowCommand>("show", ledManager);
}
}
