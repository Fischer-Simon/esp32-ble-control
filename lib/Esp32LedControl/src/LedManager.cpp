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
#include "LedStrUtils.h"

HsbColor HslToHsb(const HslColor& hsl) {
    float h = hsl.H; // Hue stays the same
    float s_h = hsl.S; // HSL Saturation
    float l = hsl.L; // Lightness in HSL

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

LedManager::LedManager(std::shared_ptr<KeyValueStore> keyValueStore, std::shared_ptr<Led::ColorManager> colorManager,
                       const std::shared_ptr<Js>& js)
    : m_keyValueStore{std::move(keyValueStore)},
      m_colorManager{std::move(colorManager)},
      m_modelLocation{
          m_keyValueStore->createValue("Settings", "ModelLocation", true, ModelLocation{0, 0, 0, 0})
      },
      m_manualModeOnStartup{
          m_keyValueStore->createValue("Settings", "StartManual", true, false)
      },
      m_manualMode{
          m_keyValueStore->createValue("Settings", "ManualMode", false, m_manualModeOnStartup->value())
      },
      m_startupAnimationEnabled{
          m_keyValueStore->createValue("Settings", "StartAnimOn", true, true)
      },
      m_js{js} {
}

std::shared_ptr<LedView> LedManager::getLedViewByName(const std::string& name) {
    return m_ledViews.get(name.c_str());
}

void LedManager::addLedView(const std::string& name, std::shared_ptr<LedView> ledView) {
    if (m_ledViews.get(name.c_str()) != nullptr) {
        log_e("LedView '%s' already exists", name.c_str());
        return;
    }
    m_ledViews.set(name.c_str(), std::move(ledView));
}

void LedManager::stopAllAnimations() const {
    std::unique_lock<std::mutex> lock{m_mutex};
    for (auto& ledString : m_ledStrings) {
        ledString->endAllAnimations();
    }
}

void LedManager::setManualMode(bool enable, bool save) const {
    std::unique_lock<std::mutex> lock{m_mutex};

    if (save && enable != m_manualModeOnStartup->value()) {
        m_manualModeOnStartup->setValue(enable);
    }

    if (enable == m_manualMode->value()) {
        return;
    }

    m_manualMode->setValue(enable);

    if (!enable) {
        m_js->runIdleAnimationStartHandlers();
        return;
    }

    m_js->rejectAllDelays();
    lock.unlock();
    stopAllAnimations();
}

void LedManager::addConfigErrorView(const std::string& name, const std::string& configKey, const std::string& error) {
    addLedView(
        name,
        std::make_shared<InvalidLedView>(
            m_keyValueStore,
            m_colorManager,
            "Missing or invalid config '" + configKey + "'" + (error.empty() ? "" : (": " + error))
        )
    );
}
