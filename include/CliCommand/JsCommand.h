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
#include <Esp32Cli/CommandGroup.h>
#include <Js.h>

namespace CliCommand {
class JsCommandGroup : public Esp32Cli::CommandGroup {
public:
    explicit JsCommandGroup(std::shared_ptr<Js> js, std::shared_ptr<Esp32Cli::Cli> cli);

private:
    static jerry_value_t jsCliHandler(const jerry_call_info_t* call_info_p,
                                  const jerry_value_t arguments[],
                                  const jerry_length_t argument_count);
    static std::shared_ptr<Esp32Cli::Cli> jsCli;
};
}
