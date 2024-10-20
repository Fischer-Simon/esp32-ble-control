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

// trim from start (in place)
inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

inline void trim(std::string& s) {
    ltrim(s);
    rtrim(s);
}

inline std::string trim_copy(std::string s) {
    ltrim(s);
    rtrim(s);
    return s;
}

HsbColor HslToHsb(const HslColor& hsl) {
    float h = hsl.H;  // Hue stays the same
    float s_h = hsl.S; // HSL Saturation
    float l = hsl.L;  // Lightness in HSL

    // Calculate Brightness (B) in HSB
    float b = l + s_h * (l * (1.0f - l));

    // Calculate Saturation (S_b) in HSB
    float s_b;
    if (l <= 0.5f) {
        s_b = (s_h * l) / (1.0f - l + s_h);
    } else {
        s_b = (s_h * (1.0f - l)) / (l + s_h);
    }

    // Return the converted HsbColor
    return {h, s_b, b};
}

std::shared_ptr<LedView> LedManager::getLedViewByName(const std::string& name) {
    auto it = m_ledViews.find(name);
    if (it == m_ledViews.end()) {
        return nullptr;
    }
    return it->second;
}

void LedManager::addLedView(std::string name, std::shared_ptr<LedView> ledView) {
    if (m_ledViews.find(name) != m_ledViews.end()) {
        log_e("LedView '%s' already exists", name.c_str());
        return;
    }
    m_ledViews.emplace(std::move(name), std::move(ledView));
}

HslColor LedManager::parseColor(const std::string& colorString, const std::shared_ptr<LedView>& ledView) const {
    size_t bracketPosition = colorString.find_first_of('(');
    size_t bracket2Position = colorString.find_first_of(')');
    std::string colorType;
    std::string colorValueStr;
    if (bracketPosition == std::string::npos || bracket2Position == std::string::npos) {
        colorType = colorString;
    } else {
        colorType = colorString.substr(0, bracketPosition);
        colorValueStr = colorString.substr(bracketPosition + 1, bracket2Position - bracketPosition - 1);
    }

    trim(colorType);
    trim(colorValueStr);

    if (colorType == "rgb") {
        size_t firstCommaPosition = colorValueStr.find_first_of(',');
        size_t lastCommaPosition = colorValueStr.find_last_of(',');
        if (firstCommaPosition == std::string::npos || lastCommaPosition == std::string::npos) {
            return {0, 0, 0};
        }
        float r = strtof(colorValueStr.substr(0, firstCommaPosition).c_str(), nullptr);
        float g = strtof(
                colorValueStr.substr(firstCommaPosition + 1, lastCommaPosition - firstCommaPosition - 1).c_str(),
                nullptr);
        float b = strtof(colorValueStr.substr(lastCommaPosition + 1, std::string::npos).c_str(), nullptr);
        return Rgb48Color{static_cast<uint16_t>(r * 65535), static_cast<uint16_t>(g * 65535),
                          static_cast<uint16_t>(b * 65535)};
    }

    if (colorType == "hsv" || colorType == "hsl") {
        size_t firstCommaPosition = colorValueStr.find_first_of(',');
        size_t lastCommaPosition = colorValueStr.find_last_of(',');
        if (firstCommaPosition == std::string::npos || lastCommaPosition == std::string::npos) {
            return {0, 0, 0};
        }
        float h = strtof(colorValueStr.substr(0, firstCommaPosition).c_str(), nullptr);
        float s = strtof(
                colorValueStr.substr(firstCommaPosition + 1, lastCommaPosition - firstCommaPosition - 1).c_str(),
                nullptr);
        float l = strtof(colorValueStr.substr(lastCommaPosition + 1, std::string::npos).c_str(), nullptr);
        return colorType == "hsv" ? HslColor{Rgb48Color{HsbColor{h, s, l}}} : HslColor{h, s, l};
    }

    HslColor baseColor;
    if (colorType == "primary") {
        if (!ledView) {
            return {0, 0, 0};
        }
        baseColor = ledView->getPrimaryColor();
    } else {
        auto it = m_namedColors.find(colorType);
        if (it == m_namedColors.end()) {
            return {0, 0, 0};
        }
        baseColor = it->second;
    }

    float v = colorValueStr.empty() ? 1.f : strtof(colorValueStr.c_str(), nullptr);
    return {baseColor.H, baseColor.S, baseColor.L * v};
}

void LedManager::addConfigErrorView(std::string name, const std::string& configKey, const std::string& error) {
    m_ledViews.emplace(std::move(name), std::make_shared<InvalidLedView>("Missing or invalid config '" + configKey + "'" + (error.empty() ? "" : (": " + error))));
}
