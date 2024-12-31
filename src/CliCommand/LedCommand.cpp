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

#include "LedStrUtils.h"

#include <Esp32Cli.h>
#include <LedManager.h>

#define LED_VIEW_FROM_FIRST_ARG m_ledManager->getLedViewByName(argv[1]);\
    if (ledView == nullptr) {\
        io.printf("Unknown LED view '%s'\n", argv[1].c_str());\
        return;\
    }\
    do {} while(0)


namespace CliCommand {
LedCommand::LedCommand(const std::shared_ptr<LedManager>& ledManager)
    : m_ledManager{ledManager} {
}

std::tuple<float, float, float> LedCommand::parsePosition(std::string positionStr) {
    ltrim(positionStr, '[');
    rtrim(positionStr, ']');
    auto parts = split_str(positionStr, ',');
    if (parts.size() != 3) {
        return {0, 0, 0};
    }
    return {
        strtof(parts[0].c_str(), nullptr),
        strtof(parts[1].c_str(), nullptr),
        strtof(parts[2].c_str(), nullptr)
    };
}

void LsCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                        const std::shared_ptr<Esp32Cli::Client>& client) const {
    for (const auto& led: m_ledManager->getLedViews()) {
        io.printf("%16s %16s  %s\n\r", led.first, led.second ? led.second->getType() : "???",
                  led.second ? led.second->getName().c_str() : "???");
    }
}

void CatCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                         const std::shared_ptr<Esp32Cli::Client>& client) const {
    if (argv.size() != 2) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledView = LED_VIEW_FROM_FIRST_ARG;
    for (int i = 0; i < ledView->getLedCount(); i++) {
        auto pixel = ledView->getLedColor(i);
        io.printf("%i(%i,%i,%i) ", i, pixel.R, pixel.G, pixel.B);
    }
    io.println();
}

void DebugCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                           const std::shared_ptr<Esp32Cli::Client>& client) const {
    if (argv.size() != 2) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledView = LED_VIEW_FROM_FIRST_ARG;
}

void GetPosCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                            const std::shared_ptr<Esp32Cli::Client>& client) const {
    auto modelLocation = m_ledManager->getModelLocation();
    if (argv.size() == 1) {
        io.printf(
            R"({"x": %f, "y": %f, "z": %f, "a": %f})" "\n",
            modelLocation.x, modelLocation.y, modelLocation.z, modelLocation.angleRad * 180.f / static_cast<float>(PI)
        );
        return;
    }
    auto ledView = LED_VIEW_FROM_FIRST_ARG;
    if (!ledView->isPositionAware()) {
        io.printf("Only physical LED strings have locations");
        return;
    }
    auto ledStringLocation = ledView->getPosition();
    if (argv.size() == 2) {
        io.printf("Local: %i, %i, %i\nGlobal: %f, %f, %f\n", ledStringLocation.x, ledStringLocation.y,
                  ledStringLocation.z, 0.f, 0.f, 0.f);
        return;
    }
    if (argv.size() == 3) {
        auto ledPosition = ledView->getLedPosition(strtoul(argv[2].c_str(), nullptr, 0));
        float x = static_cast<float>(ledStringLocation.x + ledPosition.x);
        float y = static_cast<float>(ledStringLocation.y + ledPosition.y);
        float z = static_cast<float>(ledStringLocation.z + ledPosition.z);

        float cosA = cosf(modelLocation.angleRad);
        float sinA = sinf(modelLocation.angleRad);

        float relativeX = x * cosA - y * sinA;
        float relativeY = x * sinA + y * cosA;
        float relativeZ = z;

        float globalX = relativeX - modelLocation.x;
        float globalY = relativeY - modelLocation.y;
        float globalZ = relativeZ - modelLocation.z;

        io.printf("Local: %i, %i, %i\nModel Relative: %f, %f, %f\nGlobal: %f, %f, %f\n", ledPosition.x, ledPosition.y,
                  ledPosition.z, relativeX, relativeY, relativeZ, globalX, globalY, globalZ);
        return;
    }
    Esp32Cli::Cli::printUsage(io, commandName, *this);
}

void SetPosCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                            const std::shared_ptr<Esp32Cli::Client>& client) const {
    if (argv.size() != 3) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto position = parsePosition(argv[1]);
    float rotation = strtof(argv[2].c_str(), nullptr);
    m_ledManager->setModelLocation({
        std::get<0>(position),
        std::get<1>(position),
        std::get<2>(position),
        rotation * (static_cast<float>(PI) / 180.f)
    });
}

void SetBrightnessCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                                   const std::shared_ptr<Esp32Cli::Client>& client) const {
    if (argv.size() != 3) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledView = LED_VIEW_FROM_FIRST_ARG;
    float brightness = strtof(argv[2].c_str(), nullptr);
    ledView->setBrightness(brightness);
    auto color=  ledView->getAnimationTargetColor();
    color.setBrightness(1);
    std::unique_ptr<AnimationConfig> animation{
        new AnimationConfig{color}
    };

    animation->blending = Led::Blending::Blend;
    animation->ledDuration = Led::Animation::parseDuration("150ms");

    ledView->addAnimation(std::move(animation));
}

void SetPixelCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                              const std::shared_ptr<Esp32Cli::Client>& client) const {
    if (argv.size() != 4) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledView = LED_VIEW_FROM_FIRST_ARG;
    Led::Led::index_t ledIndex = strtoul(argv[2].c_str(), nullptr, 0);
    const std::string& ledColor = argv[3];
    std::unique_ptr<AnimationConfig> animation{
        new AnimationConfig{
            ledColor,
        }
    };
    animation->leds = {ledIndex};

    ledView->addAnimation(std::move(animation));
}

void AnimatePixelCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                                  const std::shared_ptr<Esp32Cli::Client>& client) const {
    if (argv.size() != 6) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledView = LED_VIEW_FROM_FIRST_ARG;
    Led::Led::index_t ledIndex = strtoul(argv[2].c_str(), nullptr, 0);
    auto delay = Led::Animation::parseDuration(argv[3]).eval(1);
    auto duration = Led::Animation::parseDuration(argv[4]);
    const std::string& ledColor = argv[5];
    std::unique_ptr<AnimationConfig> animation{
        new AnimationConfig{
            ledColor,
        }
    };
    animation->startDelay = delay;
    animation->ledDuration = duration;
    animation->leds = {ledIndex};

    ledView->addAnimation(std::move(animation));
}

void AnimateCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                             const std::shared_ptr<Esp32Cli::Client>& client) const {
    if (argv.size() != 6 && argv.size() != 9) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledView = LED_VIEW_FROM_FIRST_ARG;
    auto delay = Led::Animation::parseDuration(argv[2]).eval(1);
    auto ledDelay = Led::Animation::parseDuration(argv[3]);
    auto duration = Led::Animation::parseDuration(argv[4]);
    const std::string& ledColor = argv[5];

    std::unique_ptr<AnimationConfig> animation{
        new AnimationConfig{
            ledColor,
        }
    };
    animation->startDelay = delay;
    animation->ledDelay = ledDelay;
    animation->ledDuration = duration;
    if (argv.size() == 9) {
        int8_t halfCycles = static_cast<int8_t>(strtol(argv[6].c_str(), nullptr, 0));
        const std::string& blendFunc = argv[7];
        const std::string& easeFunc = argv[8];

        Led::Blending blending = Led::Blending::Blend;
        if (blendFunc == "add") {
            blending = Led::Blending::Add;
        }

        animation->blending = blending;
        animation->easing = Easing::getFuncByName(easeFunc);
        animation->halfCycles = halfCycles;
    }

    ledView->addAnimation(std::move(animation));
}

void Animate3DCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                               const std::shared_ptr<Esp32Cli::Client>& client) const {
    if (argv.size() != 9 && argv.size() != 12) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }

    /*
     *led animate-3d Mirror 2,2,0 0 [200,250] [400,1000] red 64 blendManual easeLinear;led animate-3d Mirror 14,2,0 0 [200,250] [400,1000] blue 64 blendManual easeLinear
     */

    auto ledView = LED_VIEW_FROM_FIRST_ARG;
    bool positionIsLocal = argv[2] == "local";
    auto position = parsePosition(argv[3]);
    auto delay = Led::Animation::parseDuration(argv[4]).eval(1);
    auto ledDelay = Led::Animation::parseDuration(argv[5]);
    auto duration = Led::Animation::parseDuration(argv[6]);
    float range = strtof(argv[7].c_str(), nullptr);
    const std::string& ledColor = argv[8];

    std::unique_ptr<AnimationConfig> animation{
        new AnimationConfig{
            ledColor,
        }
    };
    animation->animationType = LedView::AnimationType::Wave3D;
    animation->startDelay = delay;
    animation->ledDelay = ledDelay;
    animation->ledDuration = duration;
    animation->startPos = position;
    if (!positionIsLocal) {
        animation->modelLocation = m_ledManager->getModelLocation();
    }
    animation->range = range;

    if (argv.size() == 12) {
        int8_t halfCycles = static_cast<int8_t>(strtol(argv[9].c_str(), nullptr, 0));
        const std::string& blendFunc = argv[10];
        const std::string& easeFunc = argv[11];

        Led::Blending blending = Led::Blending::Blend;
        if (blendFunc == "add") {
            blending = Led::Blending::Add;
        }

        animation->blending = blending;
        animation->easing = Easing::getFuncByName(easeFunc);
        animation->halfCycles = halfCycles;
    }

    ledView->addAnimation(std::move(animation));
}

void AnimationTimeLeftCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
    const std::shared_ptr<Esp32Cli::Client>& client) const {
    if (argv.size() != 2) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    auto ledView = LED_VIEW_FROM_FIRST_ARG;
    auto timeDiff = ledView->getCurrentAnimationEnd() - std::chrono::system_clock::now();
    io.println(std::max(0ll, std::chrono::duration_cast<std::chrono::milliseconds>(timeDiff).count()));
}

void WriteCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                           const std::shared_ptr<Esp32Cli::Client>& client) const {
}

void ShowCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                          const std::shared_ptr<Esp32Cli::Client>& client) const {
}

void ManualCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
    const std::shared_ptr<Esp32Cli::Client>& client) const {
    if (argv.size() != 2 && (argv.size() != 3 || argv[2] != "save")) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    if (argv[1] != "on" && argv[1] != "off") {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }
    m_ledManager->setManualMode(argv[1] == "on", argv.size() == 3);
}

void StartupAnimationCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
    const std::shared_ptr<Esp32Cli::Client>& client) const {
    if (argv.size() == 1) {
        m_ledManager->stopAllAnimations();
        bool isManualControl = m_ledManager->isManualMode();
        m_ledManager->setManualMode(true);
        argv = {"animate", "All", "0", "0", "1", "rgb(0,0,0)"};
        AnimateCommand{m_ledManager}.execute(io, "animate", argv, client);
        m_js->runScript("import {main} from '/data/bin/main.js'; main()", io);
        m_ledManager->setManualMode(isManualControl);
        return;
    }

    if (argv.size() != 2 || (argv[1] != "on" && argv[1] != "off")) {
        Esp32Cli::Cli::printUsage(io, commandName, *this);
        return;
    }

    m_ledManager->setStartupAnimationEnabled(argv[1] == "on");
}

LedCommandGroup::LedCommandGroup(const std::shared_ptr<LedManager>& ledManager, const std::shared_ptr<Js>& js) {
    addCommand<LsCommand>("ls", ledManager);
    addCommand<CatCommand>("cat", ledManager);
    addCommand<DebugCommand>("debug", ledManager);
    addCommand<GetPosCommand>("get-pos", ledManager);
    addCommand<SetPosCommand>("set-pos", ledManager);
    addCommand<SetBrightnessCommand>("set-brightness", ledManager);
    addCommand<SetPixelCommand>("set-pixel", ledManager);
    addCommand<AnimatePixelCommand>("animate-pixel", ledManager);
    addCommand<AnimateCommand>("animate", ledManager);
    addCommand<Animate3DCommand>("animate-3d", ledManager);
    addCommand<AnimationTimeLeftCommand>("animation-time-left", ledManager);
    addCommand<WriteCommand>("write", ledManager);
    addCommand<ShowCommand>("show", ledManager);
    addCommand<ManualCommand>("manual", ledManager);
    addCommand<StartupAnimationCommand>("startup-animation", ledManager, js);
}
}
