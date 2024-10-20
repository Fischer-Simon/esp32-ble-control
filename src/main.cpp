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

#include <condition_variable>
#include <esp_task_wdt.h>
#include <esp_ota_ops.h>
#include <LittleFS.h>
#include <mutex>
#include <WiFi.h>
#include <Esp32Cli.h>
#include <Esp32BleUi.h>
#include <Esp32DeltaOta.h>
#include <LedString.h>
#include <LedManager.h>
#include <Lua.h>

#if ESP32_BLE_CONTROL_ENABLE_WIFI
#include <ArduinoMultiWiFi.h>
#include <Esp32Cli/TelnetServer.h>
#include "WiFiCredentialSource.h"
#endif

#include "CliCommand/LuaCommand.h"
#include "CliCommand/LedCommand.h"

fs::LittleFSFS littleFs;
std::shared_ptr<Preferences> preferences;
#if ESP32_BLE_CONTROL_ENABLE_WIFI
std::shared_ptr<ArduinoMultiWiFi> wifiManager;
#endif
std::shared_ptr<Esp32BleUi> bleUi;
std::shared_ptr<Esp32Cli::Cli> cli;
std::shared_ptr<Esp32DeltaOta> ota;
std::shared_ptr<Lua> lua;
std::shared_ptr<LedManager> ledManager;

#if ESP32_BLE_CONTROL_ENABLE_WIFI
std::shared_ptr<Esp32Cli::TelnetServer> telnetServer;
#endif

class TestDataCommand : public Esp32Cli::Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const override {
        static const char* buf = "12345678";
        io.println("Test 13");

        if (argv.size() != 2) {
            return;
        }
        size_t size = strtol(argv[1].c_str(), nullptr, 0);
        io.printf("Sending %i bytes\n", size);
        while (size > 0) {
            Serial.printf("%i left\n", size);
            size_t writeSize = io.write(buf, std::min(size, 8u));
            size -= writeSize;
            if (writeSize == 0) {
                delay(10);
                esp_task_wdt_reset();
            }
        }
        io.println();
    }
};

class TestUploadCommand : public Esp32Cli::Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const override {
        if (argv.size() != 5) {
            return;
        }
        size_t size = strtol(argv[1].c_str(), nullptr, 0);
        size_t readSize = strtol(argv[2].c_str(), nullptr, 0);
        size_t blockSize = strtol(argv[3].c_str(), nullptr, 0);
        size_t blockDelay = strtol(argv[4].c_str(), nullptr, 0);
        if (readSize > 256) {
            return;
        }

        uint8_t buf[256];
        size_t blockCount = 0;

        while (size > 0) {
            size_t bytesRead = io.readBytes(buf, std::min(readSize, size));
            size -= bytesRead;
            for (int i = 0; i < bytesRead; i++) {
                Serial.printf("%02x", buf[i]);
            }
            if (blockCount == blockSize) {
                delay(blockDelay);
                blockCount = 0;
            }
            blockCount++;
        }
    }
};

class CrashCommand : public Esp32Cli::Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const override {
        *reinterpret_cast<int*>(0) = 42;
    }
};

[[noreturn]] void recoveryMode() {
    Serial.println("RECOVERY MODE");
    cli->setFirmwareInfo("RECOVERY MODE");
    while (true) {
        delay(1000);
    }
}

class PwmCommand : public Esp32Cli::Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) const override {
        if (argv.size() != 2) {
            Esp32Cli::Cli::printUsage(io, commandName, *this);
            return;
        }
        int dutyCycle = strtol(argv[1].c_str(), nullptr, 0);
        ledcWrite(0, dutyCycle);
        io.printf("Set duty cycle to %i\r\n", dutyCycle);
    }

    void printUsage(Print& output) const override {
        output.println("<duty_cycle>");
    }
};

template<uint8_t i, typename Key, typename First, typename... Rest>
struct TypeIndex {
    static constexpr uint8_t value = TypeIndex<i + 1, Key, Rest...>::value;
};

template<uint8_t i, typename Key, typename... Rest>
struct TypeIndex<i, Key, Key, Rest...> {
    static constexpr uint8_t value = i;
};


bool operator!=(const HslColor& a, const HslColor& b) {
    return a.H != b.H || a.S != b.S || a.L != b.L;
}

template<typename... Types>
class Metrics : public Esp32BleMetrics {
public:
    template<typename T>
    static inline constexpr auto
    typeIndex() -> uint8_t { return TypeIndex<0, T, Types...>::value; }

    template<typename T>
    void setValue(const char* metricNamespace, const char* objectName, const char* metricName, T value) {
        constexpr uint8_t type = typeIndex<T>();
        auto lock = std::unique_lock<std::mutex>(m_mutex);
        auto& metricValues = std::get<type>(m_metricValues);
        std::string nameStr = std::string{metricNamespace} + "/" + objectName + "/" + metricName;
        const char* name = nameStr.c_str();
        auto it = metricValues.find(name);
        bool changed;
        if (it == metricValues.end()) {
            metricValues.set(name, value);
            it = metricValues.find(name);
            changed = true;
        } else {
            auto& currentValue = it->second;
            changed = value != currentValue;
            currentValue = value;
        }

        if (!changed || !m_metricsTask ||
            m_changedMetricsBuffer.free() < (nameStr.size() + 1 + sizeof(type) + sizeof(value))) {
            return;
        }
        m_changedMetricsBuffer.push(reinterpret_cast<const uint8_t*>(name), nameStr.size() + 1);
        m_changedMetricsBuffer.push(&type, sizeof(type));
        m_changedMetricsBuffer.push(reinterpret_cast<const uint8_t*>(&it->second), sizeof(value));
        notifyTask();
    }

private:
    std::tuple<LightweightMap<Types>...> m_metricValues;
};

using MyMetrics = Metrics<int, float, HslColor>;

std::shared_ptr<MyMetrics> metrics;

void updateLedMetrics(const LedView& ledView) {
    metrics->setValue("led", ledView.getName().c_str(), "color", ledView.getAnimationTargetColor());
    metrics->setValue("led", ledView.getName().c_str(), "brightness", ledView.getBrightness());
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    // while (!Serial.available()) {
    //     delay(10);
    // }

    littleFs.begin(true, "/data", 8, "data");
    preferences = std::make_shared<Preferences>();
    preferences->begin("nvs");

    FILE* hostnameFile = fopen("/data/hostname", "r");
    if (hostnameFile) {
        char hostname[64];
        fgets(hostname, 64, hostnameFile);
        WiFiClass::setHostname(hostname);
        fclose(hostnameFile);
    }

    cli = Esp32Cli::Cli::create(WiFiClass::getHostname());

#if ESP32_BLE_CONTROL_ENABLE_WIFI
    wifiManager = std::make_shared<ArduinoMultiWiFi>(
        std::unique_ptr<WiFiCredentialSource>(new WiFiCredentialSource), preferences
    );
    telnetServer = std::make_shared<Esp32Cli::TelnetServer>(espCli, 23);
#endif

    bleUi = std::make_shared<Esp32BleUi>(cli);
    ota = std::make_shared<Esp32DeltaOta>(cli, preferences);

    if (ota->shouldStartRecoveryMode()) {
        recoveryMode();
    }

    metrics = std::make_shared<MyMetrics>();
    bleUi->setMetrics(metrics);

    ledManager = std::make_shared<LedManager>();
    ledManager->loadColorsFromConfig("/data/etc/colors.json");
    ledManager->loadLedsFromConfig("/data/etc/leds.json", &updateLedMetrics);
    cli->addCommand<CliCommand::LedCommandGroup>("led", ledManager);

    lua = std::make_shared<Lua>();
    cli->addCommand<CliCommand::LuaCommandGroup>("lua", lua);

    ledcSetup(0, 300000, 7);
    ledcAttachPin(2, 0);
    cli->addCommand<PwmCommand>("pwm");

    cli->addCommand<TestDataCommand>("test-data");
    cli->addCommand<TestUploadCommand>("test-upload");
    cli->addCommand<CrashCommand>("crash");


    vTaskDelete(nullptr);
}

void loop() {
}
