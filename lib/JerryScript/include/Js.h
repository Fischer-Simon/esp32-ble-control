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

#include <Arduino.h>
#include <mutex>
#include <utility>
#include <vector>
#include <freertos/timers.h>

#include "jerryscript/jerryscript.h"

class Js {
public:
    Js();

    ~Js();

    void loop();

    void rejectAllDelays();

    void runIdleAnimationStartHandlers();

    void registerGlobalFunction(const char* name, jerry_external_handler_t fn);

    void runScript(const char* script, Stream& io = Serial, const char* fileName = nullptr);

    void loadFile(const char* filename, Stream& io = Serial);

    static Stream& getGlobalIo() {
        return *globalIo;
    }

private:
    static void timerFn(TimerHandle_t);

    static jerry_value_t delayHandler(const jerry_call_info_t* call_info_p,
                                        const jerry_value_t arguments[],
                                        const jerry_length_t argument_count);

    static jerry_value_t printHandler(const jerry_call_info_t* call_info_p,
                                      const jerry_value_t arguments[],
                                      const jerry_length_t argument_count);

    static jerry_value_t setIdleAnimationStartHandler(const jerry_call_info_t* call_info_p,
                                      const jerry_value_t arguments[],
                                      const jerry_length_t argument_count);

    std::mutex m_mutex;
    std::vector<std::pair<uint32_t, jerry_value_t> > m_activePromises;
    std::vector<std::pair<uint32_t, jerry_value_t> > m_newPromises;
    jerry_value_t m_onIdleAnimationStartCallback;

    static Js* instance;
    static Stream* globalIo;
};
