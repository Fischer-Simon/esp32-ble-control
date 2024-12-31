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

#include "CliCommand/JsCommand.h"

#include <sstream>

namespace CliCommand {
std::shared_ptr<Esp32Cli::Cli> JsCommandGroup::jsCli{nullptr};

class StringStream : public Stream {
public:
    size_t write(uint8_t v) override {
        m_stringstream.write(reinterpret_cast<const char*>(&v), 1);
        return 1;
    }

    size_t write(const uint8_t* buffer, size_t size) override {
        m_stringstream.write(reinterpret_cast<const char*>(buffer), size);
        return size;
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

    std::string getStr() {
        return m_stringstream.str();
    }

private:
    std::stringstream m_stringstream;
};

jerry_value_t JsCommandGroup::jsCliHandler(const jerry_call_info_t* call_info_p,
                                           const jerry_value_t arguments[],
                                           const jerry_length_t argument_count) {
    if (argument_count == 0 || !jsCli) {
        return jerry_undefined();
    }

    std::vector<std::string> argv;
    for (int i = 0; i < argument_count; i++) {
        jerry_value_t stringValue = jerry_value_to_string(arguments[i]);

        jerry_char_t buffer[256];
        jerry_size_t copiedBytes =
                jerry_string_to_buffer(stringValue, JERRY_ENCODING_UTF8, buffer, sizeof (buffer) - 1);
        buffer[copiedBytes] = '\0';
        argv.emplace_back(reinterpret_cast<const char*>(buffer));
        jerry_value_free(stringValue);
    }

    StringStream stringStream;
    Stream* io = &Js::getGlobalIo();
    if (argv[0] == "return" && argv.size() > 1) {
        io = &stringStream;
        argv.erase(argv.begin());
    }

    jsCli->executeCommand(*io, argv);

    if (io == &stringStream) {
        return jerry_string_external_sz(stringStream.getStr().c_str(), nullptr);
    }
    return jerry_undefined();
}

class JsCommand : public Esp32Cli::Command {
public:
    explicit JsCommand(const std::shared_ptr<Js>& js) : m_js{js} {
    }

protected:
    std::shared_ptr<Js> m_js;
};

class MemCommand : public JsCommand {
public:
    explicit MemCommand(const std::shared_ptr<Js>& js) : JsCommand{js} {
    }

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                 const std::shared_ptr<Esp32Cli::Client>& client) const override {
        jerry_heap_stats_t heapStats;
        jerry_heap_stats(&heapStats);
        io.printf("JS Heap: %i (max %i) / %i Bytes\n", heapStats.allocated_bytes,
                  heapStats.peak_allocated_bytes, heapStats.size);
    }
};

class RunCommand : public JsCommand {
public:
    explicit RunCommand(const std::shared_ptr<Js>& js) : JsCommand{js} {
    }

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                 const std::shared_ptr<Esp32Cli::Client>& client) const override {
        if (argv.size() != 2) {
            return;
        }
        m_js->runScript(argv[1].c_str(), io);
    }

    void printUsage(Print& output) const override {
        output.println("\"<script>\")");
    }
};

class LoadCommand : public JsCommand {
public:
    explicit LoadCommand(const std::shared_ptr<Js>& js) : JsCommand{js} {
    }

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                 const std::shared_ptr<Esp32Cli::Client>& client) const override {
        if (argv.size() != 2) {
            return;
        }
        m_js->loadFile(argv[1].c_str(), io);
    }

    void printUsage(Print& output) const override {
        output.println("\"<script>\")");
    }
};

JsCommandGroup::JsCommandGroup(std::shared_ptr<Js> js, std::shared_ptr<Esp32Cli::Cli> cli) {
    jsCli = cli;
    js->registerGlobalFunction("run", &jsCliHandler);
    addCommand<MemCommand>("mem", js);
    addCommand<RunCommand>("run", js);
    addCommand<LoadCommand>("load", js);
}
}
