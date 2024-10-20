#pragma once

#include <ArduinoMultiWiFi.h>

class WiFiCredentialSource : public ArduinoMultiWiFi::CredentialSource {
public:
    explicit WiFiCredentialSource() {}

    bool hasCredential(const String& ssid) override {
        auto file = fopen((String("/data/wifi/") + ssid).c_str(), "r");
        bool hasCredential = file != nullptr;
        fclose(file);
        return hasCredential;
    }

    String getPassword(const String& ssid) override {
        auto file = fopen((String("/data/wifi/") + ssid).c_str(), "r");
        if (!file) {
            return "";
        }
        char buf[128];
        fgets(buf, sizeof buf, file);
        return buf;
    }
};
