#pragma once

#include <NeoPixelBus.h>

namespace Led {
class HslwColor {
public:
    HslwColor() = default;

    HslwColor(float brightness) : m_brightness{static_cast<uint16_t>(brightness * 255)} {
    }

    HslwColor(const HslColor& hslColor, uint8_t w, float brightness)
        : m_hslColor{hslColor}, m_w{w}, m_brightness{static_cast<uint16_t>(brightness * 255)} {
    }

    HslwColor(const HslColor& hslColor) : m_hslColor{hslColor} {
    }

    HslwColor(const Rgb48Color& rgbColor) : m_hslColor{rgbColor} {
    }

    HslwColor(const Rgb48Color& rgbColor, float brightness)
        : m_hslColor{rgbColor}, m_brightness{static_cast<uint16_t>(brightness * 255)} {
    }

    RgbwColor toRgbwColor() const {
        RgbwColor rgbw = m_hslColor;
        rgbw.W = m_w;
        if (m_brightness < 256) {
            return rgbw.Dim(m_brightness);
        }
        return rgbw.Brighten(m_brightness - 255);
    }

    const HslColor& hslColor() const {
        return m_hslColor;
    }

    uint8_t w() const {
        return m_w;
    }

    void dim(float brightness) {
        m_brightness = static_cast<uint16_t>(static_cast<float>(m_brightness) * brightness);
    }

    uint16_t brightness() const {
        return m_brightness;
    };

    void setBrightness(float brightness) {
        m_brightness = static_cast<uint16_t>(brightness * 255);
    }

private:
    HslColor m_hslColor;
    uint8_t m_w{0};
    uint16_t m_brightness{255};
};
}
