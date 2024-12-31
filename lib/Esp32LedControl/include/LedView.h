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

#include "Led/Animation.h"

#include <ArduinoJson.h>
#include <KeyValueStore.h>
#include <memory>
#include <Led/ColorManager.h>

struct ModelLocation {
    float x;
    float y;
    float z;
    float angleRad;
};

class LedManager;

class LedView {
public:
    using Animation = Led::Animation;

    enum class AnimationType {
        Linear,
        Wave3D,
    };

    struct AnimationConfig {
        static constexpr Animation::duration durationFromMs(uint16_t ms) {
            return Animation::durationFromMs(ms);
        }

        AnimationType animationType{AnimationType::Linear};
        Led::Blending blending{Led::Blending::Blend};
        ease_func_t easing{&Easing::easeLinear};
        std::string targetColorStr;
        Led::HslwColor targetColor;
        std::tuple<float, float, float> startPos; // For Wave3D
        ModelLocation modelLocation{0, 0, 0, 0};
        float range{0}; // For Wave3D
        Animation::duration startDelay{0};
        Animation::RndDuration ledDelay{}; // Or wave speed in centi units per second.
        Animation::RndDuration ledDuration{};
        int8_t halfCycles{1}; // Negative values reverse the animation
        std::vector<Led::Led::index_t> leds{};
        std::vector<std::shared_ptr<LedView> > affectedLedViews{};

        explicit AnimationConfig(std::string targetColorStr_) : targetColorStr{std::move(targetColorStr_)} {
        }

        explicit AnimationConfig(Led::HslwColor targetColor_) : targetColor{targetColor_} {
        }
    };

    explicit LedView(const std::shared_ptr<KeyValueStore>& keyValueStore,
                     const std::shared_ptr<Led::ColorManager>& colorManager, std::string name, float defaultBrightness,
                     const Led::HslwColor& primaryColor)
        : m_name{std::move(name)},
          m_colorManager{colorManager},
          m_primaryColor{primaryColor},
          m_brightness{
              keyValueStore->createValue<float>(
                  "LedBrightness", m_name.c_str(), true, defaultBrightness
              )
          },
          m_currentAnimationTargetColor{
              keyValueStore->createValue<Led::HslwColor>(
                  "LedColor", m_name.c_str(), false, Led::HslwColor{primaryColor.hslColor(), primaryColor.w(), 0}
              )
          },
          m_currentAnimationEnd{std::chrono::system_clock::now()} {
    }

    virtual ~LedView() = default;

    const std::string& getName() const {
        return m_name;
    }

    virtual const char* getType() const = 0;

    virtual bool isPositionAware() const {
        return false;
    }

    virtual Led::Position getPosition() const {
        return {0, 0, 0};
    }

    virtual Led::Position getLedPosition(Led::Led::index_t index) const {
        return {0, 0, 0};
    }

    virtual Led::Led::index_t getLedCount() const = 0;

    virtual RgbwColor getLedColor(Led::Led::index_t) const = 0;

    virtual float getBrightness() const {
        return m_brightness->value();
    }

    virtual void setBrightness(float brightness) {
        m_brightness->setValue(brightness);
    }

    bool hasPrimaryColor() const {
        return getPrimaryColor().brightness() > 0;
    }

    virtual const Led::HslwColor& getPrimaryColor() const {
        return m_primaryColor;
    }

    virtual void addAnimation(std::unique_ptr<AnimationConfig> config) = 0;

    void updateAnimationTargetColor(const Led::HslwColor& color,
                                    std::chrono::system_clock::time_point animationEnd) {
        if (animationEnd > m_currentAnimationEnd) {
            m_currentAnimationTargetColor->setValue(color);
            m_currentAnimationEnd = animationEnd;
        }
    }

    void setCurrentAnimationEnd(std::chrono::system_clock::time_point animationEnd) {
        m_currentAnimationEnd = animationEnd;
    }

    std::chrono::system_clock::time_point getCurrentAnimationEnd() const {
        return m_currentAnimationEnd;
    }

    const Led::HslwColor& getAnimationTargetColor() const {
        return m_currentAnimationTargetColor->value();
    }

    virtual void printDebug(Print&) const {
    }

protected:
    std::shared_ptr<Led::ColorManager> m_colorManager;

private:
    std::string m_name;
    Led::HslwColor m_primaryColor;
    std::shared_ptr<KeyValueStore::SimpleValue<float> > m_brightness;
    std::shared_ptr<KeyValueStore::SimpleValue<Led::HslwColor> > m_currentAnimationTargetColor;
    std::chrono::system_clock::time_point m_currentAnimationEnd;
};

class MappedLedView : public LedView, public std::enable_shared_from_this<MappedLedView> {
    struct Private {
    };

public:
    explicit MappedLedView(const std::shared_ptr<KeyValueStore>& keyValueStore,
                           const std::shared_ptr<Led::ColorManager>& colorManager, std::string name,
                           float defaultBrightness, const Led::HslwColor& primaryColor,
                           std::shared_ptr<LedView> parent,
                           std::vector<Led::Led::index_t> ledMap, Private)
        : LedView{keyValueStore, colorManager, std::move(name), defaultBrightness, primaryColor},
          m_parent{std::move(parent)},
          m_ledMap{std::move(ledMap)} {
    }

    static std::shared_ptr<MappedLedView> createInstance(const std::shared_ptr<KeyValueStore>& keyValueStore
                                                         , const std::shared_ptr<Led::ColorManager>& colorManager,
                                                         std::string name, float defaultBrightness,
                                                         const Led::HslwColor& primaryColor,
                                                         std::shared_ptr<LedView> parent,
                                                         std::vector<Led::Led::index_t> ledMap) {
        return std::make_shared<MappedLedView>(keyValueStore, colorManager, std::move(name), defaultBrightness,
                                               primaryColor,
                                               std::move(parent), std::move(ledMap), Private{});
    }

    const char* getType() const override {
        return "MapView";
    }

    Led::Led::index_t getLedCount() const override {
        return m_ledMap.size();
    }

    RgbwColor getLedColor(Led::Led::index_t i) const override {
        if (i >= m_ledMap.size()) {
            return {0};
        }
        return m_parent->getLedColor(m_ledMap[i]);
    }

    void addAnimation(std::unique_ptr<AnimationConfig> config) override;

private:
    std::shared_ptr<LedView> m_parent;
    std::vector<Led::Led::index_t> m_ledMap;
};

class MirroredLedView : public LedView, public std::enable_shared_from_this<MirroredLedView> {
    struct Private {
    };

public:
    MirroredLedView(const std::shared_ptr<KeyValueStore>& keyValueStore
                    , const std::shared_ptr<Led::ColorManager>& colorManager, std::string name,
                    std::vector<std::shared_ptr<LedView> > parents, Private)
        : LedView{
              keyValueStore, colorManager, std::move(name), parents.front()->getBrightness(),
              parents.front()->getPrimaryColor()
          },
          m_parents{std::move(parents)} {
        assert(!m_parents.empty());
    }

    static std::shared_ptr<MirroredLedView> createInstance(const std::shared_ptr<KeyValueStore>& keyValueStore
                                                           , const std::shared_ptr<Led::ColorManager>& colorManager,
                                                           std::string name,
                                                           std::vector<std::shared_ptr<LedView> > parents) {
        return std::make_shared<MirroredLedView>(keyValueStore, colorManager, std::move(name), std::move(parents),
                                                 Private{});
    }

    const char* getType() const override {
        return "MirrorView";
    }

    Led::Led::index_t getLedCount() const override {
        return m_parents.front()->getLedCount();
    }

    RgbwColor getLedColor(Led::Led::index_t i) const override {
        return m_parents.front()->getLedColor(i);
    }

    const Led::HslwColor& getPrimaryColor() const override {
        return m_parents.front()->getPrimaryColor();
    }

    void addAnimation(std::unique_ptr<AnimationConfig> config) override;

    void setBrightness(float brightness) override;

private:
    std::vector<std::shared_ptr<LedView> > m_parents;
};

class CombinedLedView : public LedView, public std::enable_shared_from_this<CombinedLedView> {
    struct Private {
    };

public:
    CombinedLedView(const std::shared_ptr<KeyValueStore>& keyValueStore,
                    const std::shared_ptr<Led::ColorManager>& colorManager, std::string name,
                    std::vector<std::shared_ptr<LedView> > parents, Private)
        : LedView{
              keyValueStore, colorManager, std::move(name), parents.front()->getBrightness(),
              parents.front()->getPrimaryColor()
          },
          m_ledCount{0},
          m_parents{std::move(parents)} {
        assert(!m_parents.empty());
        for (const auto& ledView: m_parents) {
            m_ledCount += ledView->getLedCount();
        }
    }

    static std::shared_ptr<CombinedLedView> createInstance(const std::shared_ptr<KeyValueStore>& keyValueStore,
                                                           const std::shared_ptr<Led::ColorManager>& colorManager,
                                                           std::string name,
                                                           std::vector<std::shared_ptr<LedView> > parents) {
        return std::make_shared<CombinedLedView>(keyValueStore, colorManager, std::move(name), std::move(parents),
                                                 Private{});
    }

    const char* getType() const override {
        return "CombinedView";
    }

    Led::Led::index_t getLedCount() const override {
        return m_ledCount;
    }

    RgbwColor getLedColor(Led::Led::index_t i) const override {
        Led::Led::index_t startIndex = 0;
        for (const auto& ledView: m_parents) {
            if (i - startIndex < ledView->getLedCount()) {
                return ledView->getLedColor(i - startIndex);
            }
            startIndex += ledView->getLedCount();
        }
        return {0};
    }

    const Led::HslwColor& getPrimaryColor() const override {
        return m_parents.front()->getPrimaryColor();
    }

    void addAnimation(std::unique_ptr<AnimationConfig> config) override;

    void setBrightness(float brightness) override;

private:
    Led::Led::index_t m_ledCount;
    std::vector<std::shared_ptr<LedView> > m_parents;
};

class InvalidLedView : public LedView {
public:
    explicit InvalidLedView(const std::shared_ptr<KeyValueStore>& keyValueStore,
                            const std::shared_ptr<Led::ColorManager>& colorManager, std::string name)
        : LedView{keyValueStore, colorManager, std::move(name), 0, {0.f}} {
    }

    const char* getType() const override {
        return "Invalid";
    }

    Led::Led::index_t getLedCount() const override {
        return 0;
    }

    RgbwColor getLedColor(Led::Led::index_t i) const override {
        return {0};
    }

    void addAnimation(std::unique_ptr<AnimationConfig> config) override {
    }
};
