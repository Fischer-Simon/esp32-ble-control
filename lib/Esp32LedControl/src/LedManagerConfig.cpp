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

#include "LedManager.h"

#include "LedStringNeoPixel.h"
#include "LedView.h"

#include <ArduinoJson.h>
#include <fstream>

using led_index_t = Led::Led::index_t;

#define GET_CONFIG(key, type) if (!ledConfig[#key].is<type>()) { addConfigErrorView(name, #key); continue; } auto key = ledConfig[#key].as<type>();
#define GET_CONFIG_OPTIONAL(key, type, defaultValue) auto key = ledConfig[#key].is<type>() ? ledConfig[#key].as<type>() : defaultValue;

void appendRange(std::vector<led_index_t>& indices, const std::string& range, size_t sizeLimit) {
    auto dashPosition = range.find_first_of('-');
    led_index_t startIndex = strtoul(range.substr(0, dashPosition).c_str(), nullptr, 0);
    led_index_t endIndex = (dashPosition == std::string::npos)
                               ? startIndex
                               : strtoul(range.substr(dashPosition + 1).c_str(), nullptr, 0);
    while (startIndex != endIndex && indices.size() < sizeLimit) {
        indices.push_back(startIndex);
        startIndex += startIndex > endIndex ? -1 : 1;
    };
    indices.push_back(startIndex);
}

void LedManager::loadLedsFromConfig(const std::string& ledPath) {
    JsonDocument json;
    std::ifstream file{ledPath};
    deserializeJson(json, file);
    for (const auto& i: json.as<JsonObjectConst>()) {
        std::shared_ptr<LedView> ledView;
        std::shared_ptr<LedString> ledString;
        const auto& ledConfig = i.value().as<JsonObjectConst>();
        std::string name = i.key().c_str();
        GET_CONFIG(type, JsonString);
        GET_CONFIG_OPTIONAL(primaryColor, JsonString, "");
        GET_CONFIG_OPTIONAL(defaultBrightness, JsonFloat, 1);

        Led::HslwColor primaryColorObj = primaryColor.size() != 0
                                             ? m_colorManager->parseColor(primaryColor.c_str())
                                             : Led::HslwColor{{0, 0, 0}, 0, 0};

        if (type == "NeoPixelRgb") {
            GET_CONFIG(pixelCount, JsonInteger);
            GET_CONFIG(pin, JsonInteger);
            GET_CONFIG(rmtChannel, JsonInteger);
            ledView = ledString = std::make_shared<LedStringNeoPixelRgb>(
                          m_keyValueStore, m_colorManager, name, defaultBrightness, primaryColorObj, pixelCount, pin,
                          static_cast<NeoBusChannel>(rmtChannel));
        } else if (type == "NeoPixelRgbw") {
            GET_CONFIG(pixelCount, JsonInteger);
            GET_CONFIG(pin, JsonInteger);
            GET_CONFIG(rmtChannel, JsonInteger);
            ledView = ledString = std::make_shared<LedStringNeoPixelRgbw>(
                          m_keyValueStore, m_colorManager, name, defaultBrightness, primaryColorObj, pixelCount, pin,
                          static_cast<NeoBusChannel>(rmtChannel));
        } else if (type == "NeoPixelApa104") {
            GET_CONFIG(pixelCount, JsonInteger);
            GET_CONFIG(pin, JsonInteger);
            GET_CONFIG(rmtChannel, JsonInteger);
            ledView = ledString = std::make_shared<LedStringNeoPixelApa104>(
                          m_keyValueStore, m_colorManager, name, defaultBrightness, primaryColorObj, pixelCount, pin,
                          static_cast<NeoBusChannel>(rmtChannel));
        } else if (type == "NeoPixelApa104BitBang") {
            GET_CONFIG(pixelCount, JsonInteger);
            GET_CONFIG(pin, JsonInteger);
            ledView = ledString = std::make_shared<LedStringNeoPixelApa104BitBang>(
                          m_keyValueStore, m_colorManager, name, defaultBrightness, primaryColorObj, pixelCount, pin);
        } else if (type == "MapView") {
            GET_CONFIG(parent, JsonString);
            GET_CONFIG(ledMap, JsonArrayConst);

            auto parentLedView = getLedViewByName(parent.c_str());
            if (!parentLedView) {
                addConfigErrorView(name, "parent", std::string{"LedView '"} + parent.c_str() + "' not found");
                continue;
            }
            auto parentLedCount = parentLedView->getLedCount();

            std::vector<led_index_t> ledMapVector;
            for (auto ledMapEntry: ledMap) {
                if (ledMapEntry.is<JsonInteger>()) {
                    ledMapVector.push_back(ledMapEntry.as<JsonInteger>());
                }
                if (ledMapEntry.is<JsonString>()) {
                    appendRange(ledMapVector, ledMapEntry.as<JsonString>().c_str(), parentLedCount + 1);
                }
                if (ledMapVector.size() > parentLedCount) {
                    break;
                }
            }
            if (ledMapVector.size() > parentLedCount) {
                addConfigErrorView(name, "ledMap");
                continue;
            }
            ledView = MappedLedView::createInstance(m_keyValueStore, m_colorManager, name, defaultBrightness,
                                                    primaryColorObj,
                                                    parentLedView, std::move(ledMapVector));
        } else if (type == "MirrorView") {
            GET_CONFIG(parents, JsonArrayConst);
            std::vector<std::shared_ptr<LedView> > parentVector;
            for (auto parent: parents) {
                if (!parent.is<JsonString>()) {
                    parentVector.clear();
                    break;
                }
                auto parentName = parent.as<JsonString>();
                auto parentLedView = getLedViewByName(parentName.c_str());
                if (!parentLedView) {
                    addConfigErrorView(name, "parent", std::string{"LedView '"} + parentName.c_str() + "' not found");
                    parentVector.clear();
                    break;
                }
                parentVector.push_back(parentLedView);
            }
            if (parentVector.empty()) {
                addConfigErrorView(name, "parents");
                continue;
            }
            ledView = MirroredLedView::createInstance(m_keyValueStore, m_colorManager, name, parentVector);
        } else if (type == "CombinedView") {
            GET_CONFIG(parents, JsonArrayConst);
            std::vector<std::shared_ptr<LedView> > parentVector;
            for (auto parent: parents) {
                if (!parent.is<JsonString>()) {
                    parentVector.clear();
                    break;
                }
                auto parentName = parent.as<JsonString>();
                auto parentLedView = getLedViewByName(parentName.c_str());
                if (!parentLedView) {
                    addConfigErrorView(name, "parent", std::string{"LedView '"} + parentName.c_str() + "' not found");
                    parentVector.clear();
                    break;
                }
                parentVector.push_back(parentLedView);
            }
            if (parentVector.empty()) {
                addConfigErrorView(name, "parents");
                continue;
            }
            ledView = CombinedLedView::createInstance(m_keyValueStore, m_colorManager, name, parentVector);
        }

        if (ledString && ledConfig["position"].is<JsonArrayConst>() && ledConfig["ledPositions"].is<
                JsonObjectConst>()) {
            auto position = ledConfig["position"].as<JsonArrayConst>();
            ledString->setPosition({position[0].as<int8_t>(), position[1].as<int8_t>(), position[2].as<int8_t>()});
            for (auto subString: ledConfig["ledPositions"].as<JsonObjectConst>()) {
                std::string key = subString.key().c_str();
                const size_t dashPosition = key.find('-');
                if (dashPosition != std::string::npos) {
                    if (!subString.value().is<JsonObjectConst>()) {
                        continue;
                    }
                    auto value = subString.value().as<JsonObjectConst>();
                    if (!value["offset"].is<JsonArrayConst>()) {
                        continue;
                    }
                    float x = 0;
                    float y = 0;
                    float z = 0;
                    if (value["start"].is<JsonArrayConst>()) {
                        auto start = value["start"].as<JsonArrayConst>();
                        x = start[0].as<float>();
                        y = start[1].as<float>();
                        z = start[2].as<float>();
                    }
                    auto offset = value["offset"].as<JsonArrayConst>();
                    const float dx = offset[0].as<float>();
                    const float dy = offset[1].as<float>();
                    const float dz = offset[2].as<float>();

                    led_index_t startIndex = strtoul(key.substr(0, dashPosition).c_str(), nullptr, 0);
                    led_index_t endIndex = strtoul(key.substr(dashPosition + 1).c_str(), nullptr, 0);

                    while (startIndex != endIndex) {
                        ledString->setLedPosition(
                            startIndex,
                            {static_cast<int8_t>(x), static_cast<int8_t>(y), static_cast<int8_t>(z)});
                        x += dx;
                        y += dy;
                        z += dz;

                        startIndex += startIndex > endIndex ? -1 : 1;
                    };
                    ledString->setLedPosition(startIndex, {
                                                  static_cast<int8_t>(x), static_cast<int8_t>(y), static_cast<int8_t>(z)
                                              });
                } else {
                    led_index_t index = strtoul(key.c_str(), nullptr, 0);
                    if (!subString.value().is<JsonArrayConst>()) {
                        continue;
                    }
                    auto value = subString.value().as<JsonArrayConst>();
                    ledString->setLedPosition(
                        index,
                        {value[0].as<int8_t>(), value[1].as<int8_t>(), value[2].as<int8_t>()}
                    );
                }
            }
        }

        if (!ledView) {
            ledView = std::make_shared<InvalidLedView>(
                m_keyValueStore, m_colorManager, "Unknown type '" + std::string{type.c_str()} + "'"
            );
        }
        addLedView(name, ledView);
        if (ledString) {
            m_ledStrings.emplace_back(ledString);
        }
    }
}
