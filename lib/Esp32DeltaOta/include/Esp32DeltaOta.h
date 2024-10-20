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

#include <Esp32Cli.h>
#include <Preferences.h>

#define PREF_KEY_PROGRAM_STATE "ota-state"

/**
 * Provides cli commands to allow incremental firmware updates using @link{https://github.com/eerimoq/detools}.
 */
class Esp32DeltaOta {
public:
    enum class ProgramState : uint8_t {
        New,
        RecoveryTestPending,
        RecoveryTest,
        RecoveryRequest,
        Stable,
    };

    explicit Esp32DeltaOta(std::shared_ptr<Esp32Cli::Cli> cli, std::shared_ptr<Preferences> preferences);

    /**
     * Start recovery mode after the next device restart.
     */
    void requestRecovery();

    /**
     * Indicates whether the program should start into a recovery mode where only the ota functionality is running.
     * This is true on two occasions:
     * 1. The ota client requested an update.
     * 2. The controller restarted without marking the program as good.
     * @see @link markProgramAsStable()
     * @return
     */
    bool shouldStartRecoveryMode() const;

    /**
     * Mark the current program as being in a stable state in that at least OTA updates are possible without
     * physical access to the device.
     */
    void markProgramAsStable();

    /**
     * Mark the current program as being new. If the ESP restarts with a program marked as new the flag
     * @link ProgramState::RecoveryTest will be added. Another reset should be performed which must then start
     * the safe recovery mode.  If another restart happens without the program marked as stable a rollback will
     * be performed to boot the last working firmware
     */
    void markProgramAsNew();

    bool programMarkedAsStable() const {
        return m_programState == ProgramState::Stable;
    }

    ProgramState getStartedProgramState() const {
        return m_startedProgramState;
    }

    /**
     * Perform a delayed (200 ms) reset.
     */
    static void resetEsp();

private:
    void setProgramState(ProgramState state);

    std::shared_ptr<Esp32Cli::Cli> m_cli;
    std::shared_ptr<Preferences> m_preferences;
    ProgramState m_startedProgramState;
    ProgramState m_programState;
};
