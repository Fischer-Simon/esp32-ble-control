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

#include "LedView.h"

#include <LedManager.h>

void MappedLedView::addAnimation(AnimationConfig config) {
    config.affectedLedViews.emplace_back(shared_from_this());
    config.targetBrightness *= getBrightness();
    if (config.leds.empty()) {
        config.leds = m_ledMap;
    } else {
        for (Led::Led::index_t& i : config.leds) {
            if (i >= m_ledMap.size()) {
                i = Led::Led::InvalidIndex;
                continue;
            }
            i = m_ledMap[i];
        }
    }
    m_parent->addAnimation(std::move(config));
}

void MirroredLedView::addAnimation(AnimationConfig config) {
    config.affectedLedViews.emplace_back(shared_from_this());
    for (size_t i = 0; i < m_parents.size() - 1; i++) {
        m_parents[i]->addAnimation(config);
    }
    m_parents.back()->addAnimation(std::move(config));
}

void MirroredLedView::setBrightness(float brightness) {
    for (auto& parent : m_parents) {
        parent->setBrightness(brightness);
    }
}

float MirroredLedView::getBrightness() const {
    return m_parents.front()->getBrightness();
}