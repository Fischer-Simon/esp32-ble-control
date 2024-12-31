#include "Led/ColorManager.h"

#include <fstream>
#include <ArduinoJson.h>
#include <LedStrUtils.h>

namespace Led {

void ColorManager::loadColorsFromConfig(const std::string& namedColorPath) {
    JsonDocument json;
    std::ifstream file{namedColorPath};
    deserializeJson(json, file);
    for (const auto& color: json.as<JsonObjectConst>()) {
        m_namedColors.set(color.key().c_str(), parseColor(color.value()));
    }
}

HslwColor ColorManager::parseColor(const std::string& colorString, const HslwColor& primaryColor) const {
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
            return {0};
        }
        float r = strtof(colorValueStr.substr(0, firstCommaPosition).c_str(), nullptr);
        float g = strtof(
            colorValueStr.substr(firstCommaPosition + 1, lastCommaPosition - firstCommaPosition - 1).c_str(),
            nullptr);
        float b = strtof(colorValueStr.substr(lastCommaPosition + 1, std::string::npos).c_str(), nullptr);
        float max = std::max(r, std::max(g, b));
        float brightness = 1;
        if (max > 1) {
            brightness = max;
            r /= max;
            g /= max;
            b /= max;
        }
        return {Rgb48Color{
            static_cast<uint16_t>(r * 65535), static_cast<uint16_t>(g * 65535),
            static_cast<uint16_t>(b * 65535)
        }, brightness};
    }
    if (colorType == "w") {
        return HslwColor{{0, 0, 0}, static_cast<uint8_t>(255 * strtof(colorValueStr.c_str(), nullptr)), 1};
    }

    if (colorType == "hsv" || colorType == "hsl") {
        size_t firstCommaPosition = colorValueStr.find_first_of(',');
        size_t lastCommaPosition = colorValueStr.find_last_of(',');
        if (firstCommaPosition == std::string::npos || lastCommaPosition == std::string::npos) {
            return {0};
        }
        float h = strtof(colorValueStr.substr(0, firstCommaPosition).c_str(), nullptr);
        float s = strtof(
            colorValueStr.substr(firstCommaPosition + 1, lastCommaPosition - firstCommaPosition - 1).c_str(),
            nullptr);
        float l = strtof(colorValueStr.substr(lastCommaPosition + 1, std::string::npos).c_str(), nullptr);
        return colorType == "hsv" ? HslColor{Rgb48Color{HsbColor{h, s, l}}} : HslColor{h, s, l};
    }

    HslwColor baseColor;
    if (colorType == "primary") {
        baseColor = primaryColor;
    } else {
        auto it = m_namedColors.find(colorType.c_str());
        if (it == m_namedColors.end()) {
            return {0};
        }
        baseColor = it->second;
    }

    float v = colorValueStr.empty() ? 1.f : strtof(colorValueStr.c_str(), nullptr);
    return {baseColor.hslColor(), static_cast<uint8_t>(static_cast<float>(baseColor.w())), v};
}
}
