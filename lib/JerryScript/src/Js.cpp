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

// ReSharper disable CppDFAConstantParameter
// ReSharper disable CppDFAConstantConditions
#include "Js.h"

#include <algorithm>
#include <Arduino.h>

Js* Js::instance = nullptr;
Stream* Js::globalIo = nullptr;

Js::Js() {
    assert(instance == nullptr);

    globalIo = &Serial;
    instance = this;

    jerry_init(JERRY_INIT_EMPTY);
    registerGlobalFunction("print", &printHandler);
    registerGlobalFunction("delay", &delayHandler);
    registerGlobalFunction("setIdleAnimationStartHandler", &setIdleAnimationStartHandler);

    m_onIdleAnimationStartCallback = jerry_undefined();
}

Js::~Js() {
    jerry_cleanup();
    instance = nullptr;
}

void Js::loop() {
    std::unique_lock<std::mutex> lock{m_mutex};
    if (!m_newPromises.empty()) {
        for (auto& promise: m_newPromises) {
            m_activePromises.emplace_back(promise);
        }
        m_newPromises.clear();
        std::sort(
            m_activePromises.begin(), m_activePromises.end(),
            [](std::pair<uint32_t, jerry_value_t>& a, std::pair<uint32_t, jerry_value_t>& b) {
                return a.first < b.first;
            });
    }
    while (!m_activePromises.empty() && m_activePromises.front().first < millis()) {
        jerry_value_t resolve_result = jerry_promise_resolve(m_activePromises.front().second, jerry_undefined());
        if (jerry_value_is_exception(resolve_result)) {
            jerry_value_t value_from_error = jerry_exception_value(resolve_result, true);
            printHandler(nullptr, &value_from_error, 1);
            jerry_value_free(value_from_error);
        }
        jerry_value_free(m_activePromises.front().second);
        m_activePromises.erase(m_activePromises.begin());
    }
    jerry_value_t job_value = jerry_run_jobs();
    if (jerry_value_is_exception(job_value)) {
        Serial.println("Failed to run jobs");
    }
    jerry_value_free(job_value);
}

void Js::rejectAllDelays() {
    std::unique_lock<std::mutex> lock{m_mutex};
    for (auto& promise: m_newPromises) {
        jerry_promise_reject(promise.second, jerry_undefined());
        jerry_value_free(promise.second);
    }
    m_newPromises.clear();
    for (auto& promise: m_activePromises) {
        jerry_promise_reject(promise.second, jerry_undefined());
        jerry_value_free(promise.second);
    }
    m_activePromises.clear();
}

void Js::runIdleAnimationStartHandlers() {
    std::unique_lock<std::mutex> lock{m_mutex};
    if (jerry_value_is_function(m_onIdleAnimationStartCallback)) {
        auto result = jerry_call(m_onIdleAnimationStartCallback, jerry_undefined(), nullptr, 0);
        jerry_value_free(result);
    }
}

void Js::registerGlobalFunction(const char* name, jerry_external_handler_t fn) {
    std::unique_lock<std::mutex> lock{m_mutex};
    jerry_value_t globalObject = jerry_current_realm();
    jerry_value_t propertyName = jerry_string_sz(name);
    jerry_value_t propertyValueFunc = jerry_function_external(fn);
    jerry_value_t setResult = jerry_object_set(globalObject, propertyName, propertyValueFunc);

    if (jerry_value_is_exception(setResult)) {
        Serial.printf("Failed to add the '%s' property\n", name);
        std::terminate();
    }

    jerry_value_free(setResult);
    jerry_value_free(propertyValueFunc);
    jerry_value_free(propertyName);
    jerry_value_free(globalObject);
}

void Js::runScript(const char* script, Stream& io, const char* fileName) {
    std::unique_lock<std::mutex> lock{m_mutex};
    globalIo = &io;

    const jerry_length_t scriptSize = strlen(script);
    jerry_parse_options_t options;
    options.options = JERRY_PARSE_MODULE | JERRY_PARSE_STRICT_MODE;
    if (fileName) {
        options.options |= JERRY_PARSE_HAS_SOURCE_NAME;
        options.source_name = jerry_string_external_sz(fileName, nullptr);
    }
    jerry_value_t parsedCode = jerry_parse(reinterpret_cast<const jerry_char_t*>(script), scriptSize, &options);

    if (fileName) {
        jerry_value_free(options.source_name);
    }

    if (jerry_value_is_exception(parsedCode)) {
        jerry_value_t valueFromError = jerry_exception_value(parsedCode, false);
        io.println("Parse error:");
        printHandler(nullptr, &valueFromError, 1);
        jerry_value_free(valueFromError);
    } else {
        jerry_value_t ret;
        if (options.options & JERRY_PARSE_MODULE) {
            ret = jerry_module_link(parsedCode, nullptr, nullptr);
            if (!jerry_value_is_exception(ret)) {
                jerry_value_free(ret);
                ret = jerry_module_evaluate(parsedCode);
            }
        } else {
            ret = jerry_run(parsedCode);
        }
        if (jerry_value_is_exception(ret)) {
            jerry_value_t valueFromError = jerry_exception_value(ret, false);
            io.println("Error:");
            printHandler(nullptr, &valueFromError, 1);
            jerry_value_free(valueFromError);
        }
        jerry_value_free(ret);
    }
    jerry_value_free(parsedCode);

    jerry_heap_gc(JERRY_GC_PRESSURE_LOW);

    globalIo = &Serial;
}

void Js::loadFile(const char* filename, Stream& io) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        io.printf("File %s not found", filename);
        return;
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    auto buffer = static_cast<char*>(malloc(size + 1));
    fread(buffer, size, 1, file);
    fclose(file);
    buffer[size] = '\0';

    runScript(buffer, io, filename);
    free(buffer);
}

jerry_value_t Js::delayHandler(const jerry_call_info_t* call_info_p,
                               const jerry_value_t arguments[],
                               const jerry_length_t argument_count) {
    if (argument_count == 0 || !jerry_value_is_number(arguments[0])) {
        return jerry_undefined();
    }
    uint32_t waitTime = jerry_value_as_uint32(arguments[0]);
    jerry_value_t promise = jerry_promise();

    instance->m_newPromises.emplace_back(millis() + waitTime, jerry_value_copy(promise));

    return promise;
}

jerry_value_t
Js::printHandler(const jerry_call_info_t* call_info_p,
                 const jerry_value_t arguments[],
                 const jerry_length_t argument_count) {
    if (argument_count == 0 || !globalIo) {
        return jerry_undefined();
    }
    jerry_value_t string_value = jerry_value_to_string(arguments[0]);

    jerry_char_t buffer[256];
    jerry_size_t copied_bytes = jerry_string_to_buffer(string_value, JERRY_ENCODING_UTF8, buffer, sizeof (buffer) - 1);
    buffer[copied_bytes] = '\0';

    jerry_value_free(string_value);

    globalIo->println(reinterpret_cast<const char*>(buffer));

    return jerry_undefined();
}

jerry_value_t Js::setIdleAnimationStartHandler(const jerry_call_info_t* call_info_p, const jerry_value_t arguments[],
    const jerry_length_t argument_count) {

    if (argument_count != 1 || !jerry_value_is_function(arguments[0])) {
        return jerry_undefined();
    }

    jerry_value_free(instance->m_onIdleAnimationStartCallback);
    instance->m_onIdleAnimationStartCallback = jerry_value_copy(arguments[0]);

    return jerry_undefined();
}
