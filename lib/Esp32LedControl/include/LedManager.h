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

#include <Js.h>
#include <KeyValueStore.h>
#include <LightweightMap.h>
#include <memory>
#include <Esp32Cli/Client.h>
#include <Led/ColorManager.h>

class LedManager {
public:
    explicit LedManager(std::shared_ptr<KeyValueStore> keyValueStore, std::shared_ptr<Led::ColorManager> colorManager, const std::shared_ptr<Js>& js);

    std::shared_ptr<LedView> getLedViewByName(const std::string& name);

    const std::vector<LightweightMap<std::shared_ptr<LedView> >::entry_t>& getLedViews() const {
        return m_ledViews.getEntries();
    }

    void addLedView(const std::string& name, std::shared_ptr<LedView> ledView);

    void loadLedsFromConfig(const std::string& ledPath);

    const ModelLocation& getModelLocation() const {
        return m_modelLocation->value();
    }

    void setModelLocation(const ModelLocation& modelLocation) const {
        m_modelLocation->setValue(modelLocation);
    }

    void stopAllAnimations() const;

    void setManualMode(bool enable, bool save = false) const;

    bool isManualMode() const {
        return m_manualMode->value();
    }

    void setStartupAnimationEnabled(bool enable) const {
        m_startupAnimationEnabled->setValue(enable);
    }

    bool isStartupAnimationEnabled() const {
        return m_startupAnimationEnabled->value();
    }

private:
    void addConfigErrorView(const std::string& name, const std::string& configKey, const std::string& error = "");

    mutable std::mutex m_mutex;
    std::shared_ptr<KeyValueStore> m_keyValueStore;
    std::shared_ptr<KeyValueStore::SimpleValue<ModelLocation>> m_modelLocation;
    std::shared_ptr<KeyValueStore::SimpleValue<bool>> m_manualModeOnStartup;
    std::shared_ptr<KeyValueStore::SimpleValue<bool>> m_manualMode;
    std::shared_ptr<KeyValueStore::SimpleValue<bool>> m_startupAnimationEnabled;
    std::shared_ptr<Led::ColorManager> m_colorManager;
    std::vector<std::shared_ptr<LedString> > m_ledStrings;
    LightweightMap<std::shared_ptr<LedView> > m_ledViews;
    std::shared_ptr<Js> m_js;
};
