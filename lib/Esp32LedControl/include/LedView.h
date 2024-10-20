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

#include <ArduinoJson.h>
#include <memory>

#include "Led/Animation.h"

class LedManager;

class LedView {
public:
    using Animation = Led::Animation;

    struct AnimationConfig {
        Led::Blending blending;
        ease_func_t easing;
        HslColor targetColor;
        float targetBrightness;
        Animation::duration startDelay;
        Animation::duration ledDelay;
        Animation::duration ledDuration;
        uint8_t halfCycles;
        std::vector<Led::Led::index_t> leds;
        std::vector<std::shared_ptr<LedView>> affectedLedViews;

        AnimationConfig(Led::Blending blending_, ease_func_t easing_, HslColor targetColor_,
                        Animation::duration startDelay_, Animation::duration ledDelay_,
                        Animation::duration ledDuration_, uint8_t halfCycles_)
                : blending{blending_},
                  easing{easing_},
                  targetColor{targetColor_},
                  targetBrightness{1.f},
                  startDelay{startDelay_},
                  ledDelay{ledDelay_},
                  ledDuration{ledDuration_},
                  halfCycles{halfCycles_} {
        }

        AnimationConfig(Led::Blending blending_, ease_func_t easing_, HslColor targetColor_,
                        Animation::duration startDelay_, Animation::duration ledDelay_,
                        Animation::duration ledDuration_)
                : AnimationConfig{blending_, easing_, targetColor_, startDelay_, ledDelay_, ledDuration_, 1} {
        }

        AnimationConfig(HslColor targetColor_, Animation::duration startDelay_, Animation::duration ledDelay_,
                        Animation::duration ledDuration_)
                : AnimationConfig{
                Led::Blending::Blend, &Easing::easeLinear, targetColor_, startDelay_, ledDelay_, ledDuration_, 1
        } {
        }

        AnimationConfig(HslColor targetColor_, Animation::duration ledDelay_, Animation::duration ledDuration_)
                : AnimationConfig{
                Led::Blending::Blend, &Easing::easeLinear, targetColor_, Animation::duration(0), ledDelay_,
                ledDuration_, 1
        } {
        }
    };

    explicit LedView(std::string name, float defaultBrightness, const HslColor& primaryColor)
            : m_name{std::move(name)},
              m_brightness{defaultBrightness},
              m_primaryColor{primaryColor},
              m_currentAnimationTargetColor{primaryColor},
              m_currentAnimationEnd{std::chrono::system_clock::now()} {
    }

    virtual ~LedView() = default;

    const std::string& getName() const {
        return m_name;
    }

    virtual const char* getType() const = 0;

    virtual Led::Led::index_t getLedCount() const = 0;

    virtual Rgb48Color getLedColor(Led::Led::index_t) const = 0;

    virtual Led::Position getLedPosition(Led::Led::index_t) const = 0;

    virtual float getBrightness() const {
        return m_brightness;
    }

    virtual void setBrightness(float brightness) {
        m_brightness = brightness;
    }

    virtual const HslColor& getPrimaryColor() const {
        return m_primaryColor;
    }

    virtual void addAnimation(AnimationConfig) = 0;

    void updateAnimationTargetColor(const HslColor& color, std::chrono::system_clock::time_point animationEnd) {
        if (animationEnd > m_currentAnimationEnd) {
            m_currentAnimationTargetColor = color;
            m_currentAnimationEnd = animationEnd;
            if (m_onCurrentAnimationTargetColorChanged) {
                m_onCurrentAnimationTargetColorChanged(*this);
            }
        }
    }

    const HslColor& getAnimationTargetColor() const {
        return m_currentAnimationTargetColor;
    }

    void setOnCurrentAnimationTargetColorChanged(std::function<void(const LedView&)> cb) {
        m_onCurrentAnimationTargetColorChanged = std::move(cb);
    }

    virtual void printDebug(Print&) const {
    }

private:
    std::string m_name;
    float m_brightness;
    HslColor m_primaryColor;
    HslColor m_currentAnimationTargetColor;
    std::chrono::system_clock::time_point m_currentAnimationEnd;
    std::function<void(const LedView&)> m_onCurrentAnimationTargetColorChanged{nullptr};
};

class MappedLedView : public LedView, public std::enable_shared_from_this<MappedLedView> {
    struct Private {};

public:
    explicit MappedLedView(std::string name, float defaultBrightness, const HslColor& primaryColor, std::shared_ptr<LedView> parent,
                           std::vector<Led::Led::index_t> ledMap, Private)
            : LedView{std::move(name), defaultBrightness, primaryColor},
              m_parent{std::move(parent)},
              m_ledMap{std::move(ledMap)} {
    }

    static std::shared_ptr<MappedLedView> createInstance(std::string name, float defaultBrightness, const HslColor& primaryColor, std::shared_ptr<LedView> parent,
                                                         std::vector<Led::Led::index_t> ledMap) {
        return std::make_shared<MappedLedView>(std::move(name), defaultBrightness, primaryColor, std::move(parent), std::move(ledMap), Private{});
    }

    const char* getType() const override {
        return "MapView";
    }

    Led::Led::index_t getLedCount() const override {
        return m_ledMap.size();
    }

    Rgb48Color getLedColor(Led::Led::index_t i) const override {
        if (i >= m_ledMap.size()) {
            return {0};
        }
        return m_parent->getLedColor(m_ledMap[i]);
    }

    Led::Position getLedPosition(Led::Led::index_t i) const override {
        if (i >= m_ledMap.size()) {
            return {0, 0, 0};
        }
        return m_parent->getLedPosition(m_ledMap[i]);
    }

    void addAnimation(AnimationConfig config) override;

private:
    std::shared_ptr<LedView> m_parent;
    std::vector<Led::Led::index_t> m_ledMap;
};

class MirroredLedView : public LedView, public std::enable_shared_from_this<MirroredLedView> {
    struct Private {};

public:
    explicit MirroredLedView(std::string name, std::vector<std::shared_ptr<LedView> > parents, Private)
            : LedView{std::move(name), 1.f, parents.front()->getPrimaryColor()},
              m_parents{std::move(parents)} {
        assert(!m_parents.empty());
    }

    static std::shared_ptr<MirroredLedView> createInstance(std::string name, std::vector<std::shared_ptr<LedView> > parents) {
        return std::make_shared<MirroredLedView>(std::move(name), std::move(parents), Private{});
    }

    const char* getType() const override {
        return "MirrorView";
    }

    Led::Led::index_t getLedCount() const override {
        return m_parents.front()->getLedCount();
    }

    Rgb48Color getLedColor(Led::Led::index_t i) const override {
        return m_parents.front()->getLedColor(i);
    }

    Led::Position getLedPosition(Led::Led::index_t) const override {
        return {0, 0, 0}; // A mirrored LED view can't get the position of individual indices.
    }

    const HslColor& getPrimaryColor() const override {
        return m_parents.front()->getPrimaryColor();
    }

    float getBrightness() const override;

    void addAnimation(AnimationConfig) override;

    void setBrightness(float brightness) override;

private:
    std::vector<std::shared_ptr<LedView> > m_parents;
};

class InvalidLedView : public LedView {
public:
    explicit InvalidLedView(std::string name)
            : LedView{std::move(name), 0, {0, 0, 0}} {
    }

    const char* getType() const override {
        return "Invalid";
    }

    Led::Led::index_t getLedCount() const override {
        return 0;
    }

    Rgb48Color getLedColor(Led::Led::index_t i) const override {
        return {0};
    }

    Led::Position getLedPosition(Led::Led::index_t) const override {
        return {0, 0, 0};
    }

    void addAnimation(AnimationConfig) override {
    }
};
