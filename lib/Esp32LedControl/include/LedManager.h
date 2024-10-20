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
#include "LedView.h"

#include <functional>
#include <map>
#include <memory>
#include <LightweightMap.h>

class LedManager {
public:
    std::shared_ptr<LedView> getLedViewByName(const std::string& name);

    const std::vector<LightweightMap<std::shared_ptr<LedView>>::entry_t>& getLedViews() const {
        return m_ledViews.getEntries();
    }

    void addLedView(const std::string& name, std::shared_ptr<LedView> ledView);

    void loadColorsFromConfig(const std::string& namedColorPath);

    void loadLedsFromConfig(const std::string& ledPath, const std::function<void(const LedView&)>& onAnimationColorChange = nullptr);

    HslColor parseColor(const std::string& colorString, const std::shared_ptr<LedView>& ledView = nullptr) const;

private:
    void addConfigErrorView(const std::string& name, const std::string& configKey, const std::string& error = "");

    LightweightMap<HslColor> m_namedColors;
    LightweightMap<std::shared_ptr<LedView>> m_ledViews;
};
