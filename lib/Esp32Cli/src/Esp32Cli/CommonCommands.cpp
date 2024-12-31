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

#include "Esp32Cli/CommonCommands.h"

#include <freertos/task.h>
#include <cinttypes>

#if ESP32_CLI_ENABLE_TELNET
#include <WiFi.h>
#endif

#if ESP32_CLI_ENABLE_FS_COMMANDS
#include <sys/dirent.h>
#include <sys/stat.h>
#endif

#include <LittleFS.h>

#include "Esp32Cli.h"

#define STATS_TICKS         pdMS_TO_TICKS(1000)
#define ARRAY_SIZE_OFFSET   5   //Increase this if print_real_time_stats returns ESP_ERR_INVALID_SIZE

namespace Esp32Cli {
void HostnameCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                              const std::shared_ptr<Client>& client) const {
    io.println(m_cli.getHostname().c_str());
}

void MemCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                         const std::shared_ptr<Client>& client) const {
    multi_heap_info_t info{};
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);

    io.printf("total_blocks = %i\r\n", info.total_blocks);
    io.printf("free_blocks = %i\r\n", info.free_blocks);
    io.printf("allocated_blocks = %i\r\n", info.allocated_blocks);
    io.printf("largest_free_block = %i\r\n", info.largest_free_block);
    io.printf("total_free_bytes = %i\r\n", info.total_free_bytes);
    io.printf("minimum_free_bytes = %i\r\n", info.minimum_free_bytes);
}

void resetTimerFn(void*) {
    ESP.restart();
}

void ResetCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                           const std::shared_ptr<Esp32Cli::Client>& client) const {
    int delayMs = 200;
    if (argv.size() == 2) {
        delayMs = strtol(argv[1].c_str(), nullptr, 0);
    }
    io.printf("Resetting in %i ms\n", delayMs);
    Serial.printf("Resetting in %i ms\n", delayMs);
    if (delayMs > 0) {
        TimerHandle_t resetTimer = xTimerCreate("reset", pdMS_TO_TICKS(delayMs), pdFALSE, nullptr, &resetTimerFn);
        xTimerStart(resetTimer, 0);
    } else {
        ESP.restart();
    }
}

#if ESP32_CLI_ENABLE_TELNET
void IpCommand::execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv) {
    io.println(WiFi.localIP());
}
#endif

#if ESP32_CLI_ENABLE_FS_COMMANDS
namespace FsCommands {
class TouchCommand : public Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& args,
                 const std::shared_ptr<Client>& client) const override {
        if (args.size() != 2) {
            Cli::printUsage(io, commandName, *this);
            return;
        }
        auto f = fopen(args[1].c_str(), "a");
        fclose(f);
    }

    void printUsage(Print& output) const override {
        output.println("<file_name>");
    }
};

class CatCommand : public Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& args,
                 const std::shared_ptr<Client>& client) const override {
        if (args.size() != 2) {
            Cli::printUsage(io, commandName, *this);
            return;
        }
        auto f = fopen(args[1].c_str(), "rb");
        if (!f) {
            io.println("Could not open file");
            return;
        }
        char buffer[256];
        size_t readSize;
        while ((readSize = fread(buffer, 1, sizeof(buffer), f)) > 0) {
            io.write(buffer, readSize);
        }
        fclose(f);
    }

    void printUsage(Print& output) const override {
        output.println("<file_name>");
    }
};

class WriteCommand : public Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& args,
                 const std::shared_ptr<Client>& client) const override {
        if (args.size() != 3) {
            Cli::printUsage(io, commandName, *this);
            return;
        }
        auto f = fopen(args[1].c_str(), "w");
        if (!f) {
            io.println("Could not open file");
            return;
        }
        size_t fileSize = std::strtoul(args[2].c_str(), nullptr, 0);
        io.printf("Reading %i bytes\r\n", fileSize);
        size_t readSize = 0;
        while (fileSize) {
            uint8_t readBuffer[256];
            const size_t chunkSize = std::min(sizeof(readBuffer), fileSize);
            readSize = io.readBytes(readBuffer, chunkSize);
            if (readSize != chunkSize) {
                io.println("Failed to read data");
                break;
            }
            if (fwrite(readBuffer, 1, readSize, f) != readSize) {
                io.println("Failed to write data");
                break;
            }
            fileSize -= chunkSize;
        }
        fclose(f);
    }

    void printUsage(Print& output) const override {
        output.println("<file_name> <size_in_bytes>");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        Cli::printUsage(output, commandName, *this);
        output.println("Read the given number of bytes and write the data to a file.");
    }
};

class LsCommand : public Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv,
                 const std::shared_ptr<Client>& client) const override {
        if (argv.size() < 2) {
            Cli::printUsage(io, commandName, *this);
            return;
        }

        bool verbose = false;
        if (argv[1] == "-l") {
            verbose = true;
        }

        struct DirEnt {
            enum Type {
                Unknown,
                Dir,
                File,
            } type;

            std::string name;
            size_t size;

            DirEnt(Type type_, std::string name_, size_t size_) : type(type_), name(name_), size(size_) {
            }
        };
        std::vector<DirEnt> entries;
        DIR* dir;
        dirent* dp;
        struct stat stat{};

        for (size_t i = verbose ? 2 : 1; i < argv.size(); i++) {
            if ((dir = opendir(argv[i].c_str())) == nullptr) {
                io.printf("Failed to open directory %s\n", argv[i].c_str());
                continue;
            }

            while ((dp = readdir(dir)) != nullptr) {
                if (verbose) {
                    printLongFormat(io, argv[i].c_str(), dp->d_name);
                } else {
                    io.print(dp->d_name);
                    io.print(' ');
                }
            }
            if (!verbose) {
                io.println();
            }

            closedir(dir);
        }
    }

    void printUsage(Print& output) const override {
        output.println("[-l] <dir_name>");
    }
private:
    void printLongFormat(Stream& io, const char* directory, const char* filename) const {
        struct stat fileStat;
        std::string fullPath = std::string{directory} + "/" + filename;

        if (stat(fullPath.c_str(), &fileStat) == -1) {
            io.printf("Error: Could not retrieve information for file %s\n", filename);
            return;
        }

        // Permissions string
        char permissions[2];
        permissions[0] = (S_ISDIR(fileStat.st_mode)) ? 'd' : '-';
        permissions[1] = '\0';

        // File size in human-readable format
        std::string size = humanReadableSize(fileStat.st_size);

        // Last modification time
        char timeBuf[20];
        struct tm* timeinfo = localtime(&fileStat.st_mtime);
        strftime(timeBuf, sizeof(timeBuf), "%b %d %H:%M", timeinfo);

        // Print everything in a single formatted line
        io.printf("%s %8s %s %s\n",
                  permissions,                        // Permissions
                  size.c_str(),                       // Human-readable file size
                  timeBuf,                            // Modification time
                  filename);                          // Filename
    }

    static std::string humanReadableSize(off_t bytes) {
        const char* suffixes[] = {"B", "K", "M", "G", "T"};
        size_t i = 0;
        float size = bytes;

        while (size >= 1024 && i < 4) {
            size /= 1024;
            ++i;
        }

        char output[10];
        std::snprintf(output, sizeof(output), "%.1f%s", size, suffixes[i]);
        return std::string(output);
    }
};

class DfCommand : public Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& args,
                 const std::shared_ptr<Client>& client) const override {
        io.printf("Filesystem     Size     Used    Avail Use%% Mounted on\r\n");
        io.printf("littlefs   %8i %8i %8i %3i%% /data\r\n", LittleFS.totalBytes(), LittleFS.usedBytes(),
                  LittleFS.totalBytes() - LittleFS.usedBytes(), 100 * LittleFS.usedBytes() / LittleFS.totalBytes());
    }
};
}

FsCommand::FsCommand() {
    addCommand<FsCommands::TouchCommand>("touch");
    addCommand<FsCommands::CatCommand>("cat");
    addCommand<FsCommands::WriteCommand>("write");
    addCommand<FsCommands::LsCommand>("ls");
    addCommand<FsCommands::DfCommand>("df");
}
#endif
}
