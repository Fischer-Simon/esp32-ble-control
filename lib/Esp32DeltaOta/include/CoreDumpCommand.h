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

#include <Esp32Cli/Command.h>

class CoreDumpCommand : public Esp32Cli::Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Esp32Cli::Client>&) const override {
        if (esp_core_dump_image_check() != ESP_OK) {
            io.println("No core dump found");
            return;
        }

        esp_core_dump_summary_t summary;
        esp_core_dump_get_summary(&summary);

        io.printf("Task: %s\r\n", summary.exc_task);
        io.printf("PC: 0x%08x\r\n", summary.exc_pc);

#ifndef CONFIG_IDF_TARGET_ARCH_RISCV
        for (int i = 0; i < summary.exc_bt_info.depth; i++) {
            io.printf("0x%08x\r\n", summary.exc_bt_info.bt[i]);
        }
#else
        for (int i = 0; i < summary.exc_bt_info.dump_size; i++) {
            io.printf("%02x", summary.exc_bt_info.stackdump[i]);
        }
        io.println();
#endif

        io.println();
        esp_core_dump_image_erase();
    }
};
