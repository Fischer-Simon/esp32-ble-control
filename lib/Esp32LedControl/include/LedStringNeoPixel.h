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

template<typename T_SPEED, typename T_CHANNEL> class NeoEsp32FlickerFreeRmtMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp32FlickerFreeRmtMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize)  :
        _sizeData(pixelCount * elementSize + settingsSize),
        _pin(pin)
    {
        construct();
    }

    NeoEsp32FlickerFreeRmtMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize, NeoBusChannel channel) :
        _sizeData(pixelCount* elementSize + settingsSize),
        _pin(pin),
        _channel(channel)
    {
        construct();
    }

    ~NeoEsp32FlickerFreeRmtMethodBase()
    {
        // wait until the last send finishes before destructing everything
        // arbitrary time out of 10 seconds
        ESP_ERROR_CHECK_WITHOUT_ABORT(rmt_wait_tx_done(_channel.RmtChannelNumber, 10000 / portTICK_PERIOD_MS));

        ESP_ERROR_CHECK(rmt_driver_uninstall(_channel.RmtChannelNumber));

        gpio_matrix_out(_pin, SIG_GPIO_OUT_IDX, false, false);
        pinMode(_pin, INPUT);

        free(_dataEditing);
        free(_dataSending);
    }

    bool IsReadyToUpdate() const
    {
        return (ESP_OK == ESP_ERROR_CHECK_WITHOUT_ABORT_SILENT_TIMEOUT(rmt_wait_tx_done(_channel.RmtChannelNumber, 0)));
    }

    void Initialize()
    {
        rmt_config_t config = {};

        config.rmt_mode = RMT_MODE_TX;
        config.channel = _channel.RmtChannelNumber;
        config.gpio_num = static_cast<gpio_num_t>(_pin);
        config.mem_block_num = 1;
        config.tx_config.loop_en = false;

        config.tx_config.idle_output_en = true;
        config.tx_config.idle_level = T_SPEED::IdleLevel;

        config.tx_config.carrier_en = false;
        config.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;

        config.clk_div = T_SPEED::RmtClockDivider;

        ESP_ERROR_CHECK(rmt_config(&config));
        ESP_ERROR_CHECK(rmt_driver_install(_channel.RmtChannelNumber, 0, ESP_INTR_FLAG_LEVEL3));
        ESP_ERROR_CHECK(rmt_translator_init(_channel.RmtChannelNumber, T_SPEED::Translate));
    }

    void Update(bool maintainBufferConsistency)
    {
        // wait for not actively sending data
        // this will time out at 10 seconds, an arbitrarily long period of time
        // and do nothing if this happens
        if (ESP_OK == ESP_ERROR_CHECK_WITHOUT_ABORT(rmt_wait_tx_done(_channel.RmtChannelNumber, 10000 / portTICK_PERIOD_MS)))
        {
            // now start the RMT transmit with the editing buffer before we swap
            ESP_ERROR_CHECK_WITHOUT_ABORT(rmt_write_sample(_channel.RmtChannelNumber, _dataEditing, _sizeData, false));

            if (maintainBufferConsistency)
            {
                // copy editing to sending,
                // this maintains the contract that "colors present before will
                // be the same after", otherwise GetPixelColor will be inconsistent
                memcpy(_dataSending, _dataEditing, _sizeData);
            }

            // swap so the user can modify without affecting the async operation
            std::swap(_dataSending, _dataEditing);
        }
    }

    bool AlwaysUpdate()
    {
        // this method requires update to be called only if changes to buffer
        return false;
    }

    bool SwapBuffers()
    {
        std::swap(_dataSending, _dataEditing);
        return true;
    }

    uint8_t* getData() const
    {
        return _dataEditing;
    };

    size_t getDataSize() const
    {
        return _sizeData;
    }

    void applySettings([[maybe_unused]] const SettingsObject& settings)
    {
    }

private:
    const size_t  _sizeData;      // Size of '_data*' buffers
    const uint8_t _pin;            // output pin number
    const T_CHANNEL _channel; // holds instance for multi channel support

    // Holds data stream which include LED color values and other settings as needed
    uint8_t*  _dataEditing;   // exposed for get and set
    uint8_t*  _dataSending;   // used for async send using RMT


    void construct()
    {
        _dataEditing = static_cast<uint8_t*>(malloc(_sizeData));
        // data cleared later in Begin()

        _dataSending = static_cast<uint8_t*>(malloc(_sizeData));
        // no need to initialize it, it gets overwritten on every send
    }
};


class LedStringNeoPixelRgb : public LedString {
public:
    LedStringNeoPixelRgb(const std::shared_ptr<KeyValueStore>& keyValueStore, const std::shared_ptr<Led::ColorManager>& colorManager, std::string description,
                         float defaultBrightness, const Led::HslwColor& primaryColor, led_index_t ledCount, uint8_t pin,
                         NeoBusChannel rmtChannel)
        : LedString{keyValueStore, colorManager, std::move(description), defaultBrightness, primaryColor, ledCount},
          m_neoPixels{ledCount, pin, rmtChannel} {
        m_neoPixels.Begin();
        m_neoPixels.Show();
    }

    const char* getType() const override {
        return "NeoPixelRgb";
    }

protected:
    void setLedColor(led_index_t index, RgbwColor color) override {
        m_neoPixels.SetPixelColor(index, NeoGamma<NeoGammaTableMethod>::Correct(RgbColor(color)));
    }

    void showLeds() override {
        m_neoPixels.Show();
    }

private:
    NeoPixelBus<NeoGrbFeature, NeoEsp32FlickerFreeRmtMethodBase<NeoEsp32RmtSpeed800Kbps, NeoEsp32RmtChannelN>> m_neoPixels;
};

class LedStringNeoPixelRgbw : public LedString {
public:
    LedStringNeoPixelRgbw(const std::shared_ptr<KeyValueStore>& keyValueStore, const std::shared_ptr<Led::ColorManager>& colorManager, std::string description,
                         float defaultBrightness, const Led::HslwColor& primaryColor, led_index_t ledCount, uint8_t pin,
                         NeoBusChannel rmtChannel)
        : LedString{keyValueStore, colorManager, std::move(description), defaultBrightness, primaryColor, ledCount},
          m_neoPixels{ledCount, pin, rmtChannel} {
        m_neoPixels.Begin();
        m_neoPixels.Show();
    }

    const char* getType() const override {
        return "NeoPixelRgbw";
    }

protected:
    void setLedColor(led_index_t index, RgbwColor color) override {
        m_neoPixels.SetPixelColor(index, NeoGamma<NeoGammaTableMethod>::Correct(color));
    }

    void showLeds() override {
        m_neoPixels.Show();
    }

private:
    NeoPixelBus<NeoGrbwFeature, NeoEsp32FlickerFreeRmtMethodBase<NeoEsp32RmtSpeed800Kbps, NeoEsp32RmtChannelN>> m_neoPixels;
};

class LedStringNeoPixelApa104 : public LedString {
public:
    LedStringNeoPixelApa104(const std::shared_ptr<KeyValueStore>& keyValueStore, const std::shared_ptr<Led::ColorManager>& colorManager, std::string description,
                         float defaultBrightness, const Led::HslwColor& primaryColor, led_index_t ledCount, uint8_t pin,
                         NeoBusChannel rmtChannel)
        : LedString{keyValueStore, colorManager, std::move(description), defaultBrightness, primaryColor, ledCount},
          m_neoPixels{ledCount, pin, rmtChannel} {
        m_neoPixels.Begin();
        m_neoPixels.Show();
    }

    const char* getType() const override {
        return "NeoPixelApa104";
    }

protected:
    void setLedColor(led_index_t index, RgbwColor color) override {
        m_neoPixels.SetPixelColor(index, NeoGamma<NeoGammaTableMethod>::Correct(RgbColor(color)));
    }

    void showLeds() override {
        m_neoPixels.Show();
    }

private:
    NeoPixelBus<NeoGrbFeature, NeoEsp32FlickerFreeRmtMethodBase<NeoEsp32RmtSpeedWs2812x, NeoEsp32RmtChannelN>> m_neoPixels;
};

class LedStringNeoPixelApa104BitBang : public LedString {
public:
    LedStringNeoPixelApa104BitBang(const std::shared_ptr<KeyValueStore>& keyValueStore, const std::shared_ptr<Led::ColorManager>& colorManager, std::string description,
                         float defaultBrightness, const Led::HslwColor& primaryColor, led_index_t ledCount, uint8_t pin)
        : LedString{keyValueStore, colorManager, std::move(description), defaultBrightness, primaryColor, ledCount},
          m_neoPixels{ledCount, pin} {
        m_neoPixels.Begin();
        m_neoPixels.Show();
    }

    const char* getType() const override {
        return "NeoPixelApa104BitBang";
    }

protected:
    void setLedColor(led_index_t index, RgbwColor color) override {
        m_neoPixels.SetPixelColor(index, NeoGamma<NeoGammaTableMethod>::Correct(RgbColor(color)));
    }

    void showLeds() override {
        m_neoPixels.Show();
    }

private:
    NeoPixelBus<NeoRgbFeature, NeoEsp32BitBangWs2812xMethod> m_neoPixels;
};
