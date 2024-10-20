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

#include "LedString.h"

#include <NeoPixelBus.h>

class LedStringNeoPixelRgb : public LedString {
public:
    LedStringNeoPixelRgb(std::string description, float defaultBrightness, const HslColor& primaryColor, led_index_t ledCount, uint8_t pin, NeoBusChannel rmtChannel)
        : LedString{std::move(description), defaultBrightness, primaryColor, ledCount},
          m_neoPixels{ledCount, pin, rmtChannel} {
        m_neoPixels.Begin();
        m_neoPixels.Show();
    }

    const char* getType() const override {
        return "NeoPixelRgb";
    }

protected:
    void setLedColor(led_index_t index, Rgb48Color color) override {
        m_neoPixels.SetPixelColor(index, NeoGamma<NeoGammaTableMethod>::Correct(RgbColor(color)));
    }

    void showLeds() override {
        m_neoPixels.Show();
    }

private:
    NeoPixelBus<NeoGrbFeature, NeoEsp32RmtN800KbpsMethod> m_neoPixels;
};
