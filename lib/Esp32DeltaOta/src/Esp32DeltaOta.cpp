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

#include "Esp32DeltaOta.h"

#include "FirmwareCommand.h"

#include <esp_core_dump.h>
#include <esp_ota_ops.h>
#include <Esp32Cli/CommandGroup.h>
#include <Esp32Cli/CommonCommands.h>
#include "CoreDumpCommand.h"

class DummyStream : public Stream {
public:
    size_t write(uint8_t) override {
        return 1;
    }

    int available() override {
        return 0;
    }

    int read() override {
        return -1;
    }

    int peek() override {
        return -1;
    }
};

Esp32DeltaOta::Esp32DeltaOta(std::shared_ptr<Esp32Cli::Cli> cli, std::shared_ptr<Preferences> preferences)
    : m_cli{std::move(cli)},
      m_preferences{std::move(preferences)} {
    m_programState = static_cast<ProgramState>(m_preferences->getUChar(
        PREF_KEY_PROGRAM_STATE, static_cast<uint8_t>(ProgramState::Stable)));
    m_startedProgramState = m_programState;

    m_cli->addCommand<Esp32DeltaOtaCommands::FirmwareCommand>(
        "firmware", *this, m_startedProgramState == ProgramState::RecoveryTest
    );
    m_cli->addCommand<CoreDumpCommand>("core-dump");

    if (m_startedProgramState == ProgramState::New || m_startedProgramState == ProgramState::RecoveryTest) {
        // Newly flashed program, updater should now test recovery boot by requesting recovery mode. Or:
        // Recovery test currently in progress.
        setProgramState(ProgramState::RecoveryTestPending);
    } else if (m_startedProgramState == ProgramState::RecoveryRequest) {
        // Recovery has been requested from a stable program.
        setProgramState(ProgramState::Stable);
    } else if (m_startedProgramState == ProgramState::RecoveryTestPending) {
        // ESP has been reset during recovery testing without marking the program as stable. Rollback now.
        Serial.println("Program restarted in an unstable state, rolling back");
        if (esp_ota_set_boot_partition(esp_ota_get_next_update_partition(nullptr)) != ESP_OK) {
            // Oh no. Roll back failed. Nothing more to do now. Just set the program as stable and hope for the best.
            Serial.println("Firmware rollback failed.");
        }
        setProgramState(ProgramState::Stable);
        ESP.restart();
    } else if (esp_core_dump_image_check() == ESP_OK) {
        m_startedProgramState = m_programState = ProgramState::RecoveryRequest;
    }
}

void Esp32DeltaOta::requestRecovery() {
    if (m_programState == ProgramState::Stable) {
        setProgramState(ProgramState::RecoveryRequest);
    }
    if (m_programState == ProgramState::RecoveryTestPending) {
        setProgramState(ProgramState::RecoveryTest);
    }
}

bool Esp32DeltaOta::shouldStartRecoveryMode() const {
    return m_startedProgramState == ProgramState::RecoveryTest
           || m_startedProgramState == ProgramState::RecoveryRequest;
}

void Esp32DeltaOta::markProgramAsStable() {
    if (m_programState == ProgramState::Stable) {
        return;
    }

    setProgramState(ProgramState::Stable);
}

void Esp32DeltaOta::markProgramAsNew() {
    setProgramState(ProgramState::New);
}

void Esp32DeltaOta::resetEsp() {
    DummyStream io;
    std::vector<std::string> argv = {"reset", "200"};
    (new Esp32Cli::ResetCommand)->execute(io, "reset", argv, nullptr);
}

void Esp32DeltaOta::setProgramState(ProgramState state) {
    m_preferences->putUChar(PREF_KEY_PROGRAM_STATE, static_cast<uint8_t>(state));
    m_programState = state;
}
