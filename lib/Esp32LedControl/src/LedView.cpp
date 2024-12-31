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

void MappedLedView::addAnimation(std::unique_ptr<AnimationConfig> config) {
    config->affectedLedViews.emplace_back(shared_from_this());
    if (!config->targetColorStr.empty() && hasPrimaryColor()) {
        config->targetColor = m_colorManager->parseColor(config->targetColorStr, getPrimaryColor());
        config->targetColorStr.clear();
    }
    config->targetColor.dim(getBrightness());
    if (config->leds.empty()) {
        config->leds = m_ledMap;
    } else {
        for (Led::Led::index_t& i : config->leds) {
            if (i >= m_ledMap.size()) {
                i = Led::Led::InvalidIndex;
                continue;
            }
            i = m_ledMap[i];
        }
    }
    m_parent->addAnimation(std::move(config));
}

void MirroredLedView::addAnimation(std::unique_ptr<AnimationConfig> config) {
    config->affectedLedViews.emplace_back(shared_from_this());
    for (size_t i = 0; i < m_parents.size() - 1; i++) {
        const auto& parent = m_parents[i];
        m_parents[i]->addAnimation(std::unique_ptr<AnimationConfig>(new AnimationConfig(*config)));
    }

    m_parents.back()->addAnimation(std::move(config));
}

void MirroredLedView::setBrightness(float brightness) {
    LedView::setBrightness(brightness);
    for (auto& parent : m_parents) {
        parent->setBrightness(brightness);
    }
}

void CombinedLedView::addAnimation(std::unique_ptr<AnimationConfig> config) {
    config->affectedLedViews.emplace_back(shared_from_this());
    Led::Led::index_t baseIndex = 0;
    for (size_t i = 0; i < m_parents.size() - 1; i++) {
        auto parentLedCount = m_parents[i]->getLedCount();
        std::unique_ptr<AnimationConfig> newConfig{new AnimationConfig(*config)};
        for (auto& ledIndex : newConfig->leds) {
            if (ledIndex < baseIndex || ledIndex >= baseIndex + parentLedCount) {
                ledIndex = Led::Led::InvalidIndex;
            } else {
                ledIndex -= baseIndex;
            }
        }
        m_parents[i]->addAnimation(std::move(newConfig));
        baseIndex += parentLedCount;
    }
    for (auto& ledIndex : config->leds) {
        if (ledIndex < baseIndex || ledIndex >= baseIndex + m_parents.back()->getLedCount()) {
            ledIndex = Led::Led::InvalidIndex;
        } else {
            ledIndex -= baseIndex;
        }
    }
    m_parents.back()->addAnimation(std::move(config));
}

void CombinedLedView::setBrightness(float brightness) {
    LedView::setBrightness(brightness);
    for (auto& parent : m_parents) {
        parent->setBrightness(brightness);
    }
}
