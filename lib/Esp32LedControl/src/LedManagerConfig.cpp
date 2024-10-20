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
    led_index_t endIndex = (dashPosition == std::string::npos) ? startIndex : strtoul(range.substr(dashPosition + 1).c_str(), nullptr, 0);
    while (startIndex != endIndex && indices.size() < sizeLimit) {
        indices.push_back(startIndex);
        startIndex += startIndex > endIndex ? -1 : 1;
    };
    indices.push_back(startIndex);
}

void LedManager::loadColorsFromConfig(const std::string& namedColorPath) {
    JsonDocument json;
    std::ifstream file{namedColorPath};
    deserializeJson(json, file);
    for (const auto& color : json.as<JsonObjectConst>()) {
        m_namedColors.set(color.key().c_str(), parseColor(color.value()));
    }
}

void LedManager::loadLedsFromConfig(const std::string& ledPath, const std::function<void(const LedView&)>& onAnimationColorChange) {
    JsonDocument json;
    std::ifstream file{ledPath};
    deserializeJson(json, file);
    for (const auto& i : json.as<JsonObjectConst>()) {
        std::shared_ptr<LedView> ledView;
        const auto& ledConfig = i.value().as<JsonObjectConst>();
        std::string name = i.key().c_str();
        GET_CONFIG(type, JsonString);
        GET_CONFIG_OPTIONAL(primaryColor, JsonString, "");
        GET_CONFIG_OPTIONAL(defaultBrightness, JsonFloat, 1);

        HslColor primaryColorObj = parseColor(primaryColor.c_str());

        if (type == "NeoPixelRgb") {
            GET_CONFIG(pixelCount, JsonInteger);
            GET_CONFIG(pin, JsonInteger);
            GET_CONFIG(rmtChannel, JsonInteger);
            ledView = std::make_shared<LedStringNeoPixelRgb>(name, defaultBrightness, primaryColorObj, pixelCount, pin, static_cast<NeoBusChannel>(rmtChannel));
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
            for (auto ledMapEntry : ledMap) {
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
            ledView = MappedLedView::createInstance(name, defaultBrightness, primaryColorObj, parentLedView, std::move(ledMapVector));
        } else if (type == "MirrorView") {
            GET_CONFIG(parents, JsonArrayConst);
            std::vector<std::shared_ptr<LedView>> parentVector;
            for (auto parent : parents) {
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
            ledView = MirroredLedView::createInstance(name, parentVector);
        }

        if (!ledView) {
            ledView = std::make_shared<InvalidLedView>("Unknown type '" + std::string{type.c_str()} + "'");
        }
        ledView->setOnCurrentAnimationTargetColorChanged(onAnimationColorChange);
        addLedView(name, ledView);
    }
}
