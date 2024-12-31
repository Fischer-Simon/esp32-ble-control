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
#include <KeyValueStore.h>

#if ESP32_BLE_CONTROL_ENABLE_WIFI
#include <ArduinoMultiWiFi.h>
#include <Esp32Cli/TelnetServer.h>
#include "WiFiCredentialSource.h"
#endif

#include "CliCommand/LedCommand.h"
#include <Js.h>
#include <nvs.h>
#include <CliCommand/JsCommand.h>

std::shared_ptr<Preferences> preferences;
#if ESP32_BLE_CONTROL_ENABLE_WIFI
std::shared_ptr<ArduinoMultiWiFi> wifiManager;
#endif
std::shared_ptr<KeyValueStore> keyValueStore;
std::shared_ptr<Esp32BleUi> bleUi;
std::shared_ptr<Esp32Cli::Cli> cli;
std::shared_ptr<Esp32DeltaOta> ota;
std::shared_ptr<Js> js;
std::shared_ptr<Led::ColorManager> colorManager;
std::shared_ptr<LedManager> ledManager;

#if ESP32_BLE_CONTROL_ENABLE_WIFI
std::shared_ptr<Esp32Cli::TelnetServer> telnetServer;
#endif

class TestDataCommand : public Esp32Cli::Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Esp32Cli::Client>& client) const override {
        static const char* buf = "12345678";

        if (argv.size() != 2) {
            return;
        }
        size_t size = strtol(argv[1].c_str(), nullptr, 0);
        io.printf("Sending %i bytes\n", size);
        while (size > 0) {
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
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Esp32Cli::Client>& client) const override {
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

class MetricsCommand : public Esp32Cli::Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Esp32Cli::Client>& client) const override {
        keyValueStore->writeAllValuesToStream(io);
    }
};

class CrashCommand : public Esp32Cli::Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Esp32Cli::Client>& client) const override {
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

#include <argparse.h>

using argparse::ArgumentParser;

class PwmCommand : public Esp32Cli::Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Esp32Cli::Client>& client) const override {
        if (argv.size() != 2) {
            Esp32Cli::Cli::printUsage(io, commandName, *this);
            return;
        }
        float dutyCycle = strtof(argv[1].c_str(), nullptr);
        update_bldc_speed(dutyCycle);
        io.printf("Set duty cycle to %f\r\n", dutyCycle);
    }

    void printUsage(Print& output) const override {
        output.println("<duty_cycle>");
    }
};

class ThermalCommand : public Esp32Cli::Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Esp32Cli::Client>& client) const override {
        io.printf("%.1f °C\n", temperatureRead());
    }
};

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    LittleFS.begin(true, "/data", 8, "data");
    preferences = std::make_shared<Preferences>();
    preferences->begin("nvs");

    char hostname[64];
    FILE* hostnameFile = fopen("/data/hostname", "r");
    if (hostnameFile) {
        fgets(hostname, 64, hostnameFile);
        fclose(hostnameFile);
    } else {
        uint8_t baseMac[6];
        esp_read_mac(baseMac, ESP_MAC_BT);
        snprintf(hostname, 64, "%s%02X%02X%02X", CONFIG_IDF_TARGET "-", baseMac[3], baseMac[4], baseMac[5]);
    }

    cli = Esp32Cli::Cli::create(hostname);

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

    srand(esp_random());

    keyValueStore = std::make_shared<KeyValueStore>();
    bleUi->setKeyValueStore(keyValueStore);

    js = std::make_shared<Js>();
    cli->addCommand<CliCommand::JsCommandGroup>("js", js, cli);

    colorManager = std::make_shared<Led::ColorManager>();
    colorManager->loadColorsFromConfig("/data/lib/colors.json");
    ledManager = std::make_shared<LedManager>(keyValueStore, colorManager, js);
    ledManager->loadLedsFromConfig("/data/etc/leds.json");
    cli->addCommand<CliCommand::LedCommandGroup>("led", ledManager, js);

    cli->addCommand<MetricsCommand>("metrics");
    cli->addCommand<TestDataCommand>("test-data");
    cli->addCommand<TestUploadCommand>("test-upload");
    cli->addCommand<CrashCommand>("crash");
    cli->addCommand<ThermalCommand>("thermal");

    if (ledManager->isStartupAnimationEnabled()) {
        std::vector<std::string> argv = {"led", "startup-animation"};
        cli->executeCommand(Serial, argv);
    }
    if (!ledManager->isManualMode()) {
        js->runIdleAnimationStartHandlers();
    }
}

void loop() {
    js->loop();
    delay(16);
}
