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

#include "FirmwareCommand.h"
#include "Esp32DeltaOta.h"

#include <Esp32Cli.h>
#include <esp_littlefs.h>
#include <Esp32Cli/CommonCommands.h>
#include <esp_ota_ops.h>

extern "C" {
#include "heatshrink/heatshrink_encoder.h"
#include "heatshrink/heatshrink_decoder.h"
#include "bspatch.h"
}

namespace Esp32DeltaOtaCommands {
typedef struct {
    const esp_partition_t* srcPartition{};
    const esp_partition_t* destPartition{};
    heatshrink_decoder* decompressor{};
    Stream* patchStream{};
    esp_ota_handle_t otaHandle{};
    size_t compressedPatchSizeLeft{0};
    size_t patchSize{0};
    size_t bytesWritten{0};
    size_t nextStatusUpdate{0};
} ota_state_t;

static int writeDest(const bspatch_stream_n* statePtr, const void* data, int size) {
    auto* state = static_cast<ota_state_t*>(statePtr->opaque);

    if (!state || size <= 0) {
        return -200;
    }

    if (esp_ota_write(state->otaHandle, data, size) != ESP_OK) {
        return -201;
    }

    state->bytesWritten += size;
    if (state->bytesWritten >= state->nextStatusUpdate && state->patchStream->availableForWrite() >= 16) {
        state->patchStream->printf(R"({"progress": %i})""\n", state->bytesWritten);
        state->nextStatusUpdate += 16384;
    }

    return 0;
}

static int readSrc(const bspatch_stream_i* statePtr, void* data, int pos, int size) {
    auto* state = static_cast<ota_state_t*>(statePtr->opaque);

    if (!state || size <= 0) {
        return -200;
    }

    if (esp_partition_read(state->srcPartition, pos, data, size) != ESP_OK) {
        return -202;
    }

    return 0;
}

static int readPatch(const bspatch_stream* statePtr, void* data, int size) {
    static constexpr size_t SINK_BUF_SIZE = 32;

    auto* state = static_cast<ota_state_t*>(statePtr->opaque);

    if (!state || size <= 0) {
        return -200;
    }

    auto* outBuf = static_cast<uint8_t*>(data);
    size_t bytesRead = 0;
    while (bytesRead < size) {
        size_t outBytes = 0;
        heatshrink_decoder_poll(state->decompressor, &outBuf[bytesRead], size - bytesRead, &outBytes);
        bytesRead += outBytes;
        if (outBytes != 0) {
            continue;
        }
        if (state->compressedPatchSizeLeft > 0) {
            uint8_t sinkBuf[SINK_BUF_SIZE];
            size_t sinkBytes = std::min(SINK_BUF_SIZE, state->compressedPatchSizeLeft);
            size_t sunkBytes;
            if (state->patchStream->readBytes(sinkBuf, sinkBytes) != sinkBytes) {
                return -203;
            }
            if (heatshrink_decoder_sink(state->decompressor, sinkBuf, sinkBytes, &sunkBytes) != HSDR_SINK_OK || sinkBytes != sunkBytes) {
                return -204;
            }
            state->compressedPatchSizeLeft -= sinkBytes;
            continue;
        }
        if (heatshrink_decoder_finish(state->decompressor) != HSDR_FINISH_MORE) {
            return -205;
        }
    }

    return 0;
}

class InfoCommand : public Esp32Cli::Command {
public:
    explicit InfoCommand(Esp32DeltaOta& ota) : m_ota{ota} {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Esp32Cli::Client>& client) const override {
        auto partition = esp_ota_get_running_partition();
        esp_partition_pos_t partitionPosition = {partition->address, partition->size};
        esp_image_metadata_t imageMetadata;
        esp_image_get_metadata(&partitionPosition, &imageMetadata);

        uint8_t digest[32];
        esp_partition_get_sha256(partition, digest);

        io.write(R"({"hash": ")");
        for (unsigned char i: digest) {
            io.printf("%02x", i);
        }
        io.write(R"(", "size": )");
        io.print(imageMetadata.image_len);
        io.print(R"(, "partition": ")");
        io.print(partition->label);
        io.print(R"(", "state": )");
        io.print(static_cast<int>(m_ota.getStartedProgramState()));
        io.println("}");
    }

private:
    Esp32DeltaOta& m_ota;
};

class EraseNextCommand : public Esp32Cli::Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Esp32Cli::Client>& client) const override {
        if (argv.size() != 2) {
            Esp32Cli::Cli::printUsage(io, commandName, *this);;
            return;
        }
        auto partition = esp_ota_get_next_update_partition(nullptr);
        int eraseSize = strtol(argv[1].c_str(), nullptr, 0);

        constexpr int eraseBlockSize = 4096;
        uint32_t blockCount = eraseSize / eraseBlockSize + ((eraseSize % eraseBlockSize != 0) ? 1 : 0);
        for (uint32_t block = 0; block < blockCount; block++) {
            uint32_t blockOffset = block * eraseBlockSize;

            esp_err_t ret = esp_partition_erase_range(partition, blockOffset, eraseBlockSize);
            if (ret != ESP_OK) {
                log_e("ESP partition erase error %i\n", ret);
                io.printf(R"({"error": "esp_partition_erase_range failed with %i"})", ret);
                return;
            }
            io.printf(R"({"currentBlock": %i, "blockCount": %i})""\n", block, blockCount);
        }
    }

    void printUsage(Print& output) const override {
        output.print("<erase_size>");
    }
};

class FlashDeltaCommand : public Esp32Cli::Command {
public:
    explicit FlashDeltaCommand(Esp32DeltaOta& ota) : m_ota{ota} {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Esp32Cli::Client>& client) const override {
        if (argv.size() != 5) {
            Esp32Cli::Cli::printUsage(io, commandName, *this);;
            return;
        }

        const int compressedPatchSize = strtol(argv[1].c_str(), nullptr, 0);
        const int patchSize = strtol(argv[2].c_str(), nullptr, 0);
        const int imageSize = strtol(argv[3].c_str(), nullptr, 0);
        const int oldImageSize = strtol(argv[4].c_str(), nullptr, 0);

        ota_state_t otaState;

        otaState.srcPartition = esp_ota_get_running_partition();
        otaState.destPartition = esp_ota_get_next_update_partition(nullptr);

        if (otaState.srcPartition == nullptr || otaState.destPartition == nullptr
            || otaState.srcPartition->subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_MAX ||
            otaState.destPartition->subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_MAX) {
            io.println(R"({"error": "Required ESP partition not found"})");
            return;
        }

        otaState.patchStream = &io;
        otaState.decompressor = heatshrink_decoder_alloc(32, 11, 9);
        otaState.compressedPatchSizeLeft = compressedPatchSize;
        otaState.patchSize = patchSize;

        Serial.println("esp_ota_begin");
        if (esp_ota_begin(otaState.destPartition, imageSize, &(otaState.otaHandle)) != ESP_OK) {
            io.println(R"({"error": "esp_ota_begin failed"})");
            return;
        }
        Serial.println("esp_ota_begin done");

        bspatch_stream_i inputStream{&otaState, readSrc};
        bspatch_stream_n outputStream{&otaState, writeDest};
        bspatch_stream patchStream{&otaState, readPatch};
        int ret = bspatch(&inputStream, oldImageSize, &outputStream, imageSize, &patchStream);
        if (ret < 0) {
            io.printf(R"({"error": "bspatch failed with %i"})""\n", ret);
            uint8_t dummy;
            while (patchStream.read(&patchStream, &dummy, 1) == 0) {
            }
            return;
        }

        ret = esp_ota_end(otaState.otaHandle);
        if (ret != ESP_OK) {
            io.printf(R"({"error": "esp_ota_end %i"})""\n", ret);
            return;
        }

        if (esp_ota_set_boot_partition(otaState.destPartition) == ESP_OK) {
            m_ota.markProgramAsNew();
            io.println(R"({"success": 1})");
        } else {
            io.println(R"({"error": "esp_ota_set_boot_partition failed"})");
        }
    }

    void printUsage(Print& output) const override {
        output.println("<compressed_patch_size> <patch_size> <image_size> <old_image_size>");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        Esp32Cli::Cli::printUsage(output, commandName, *this);
        output.println("Perform delta update. ESP doesn't automatically restart after the update.");
    }

private:
    Esp32DeltaOta& m_ota;
};

class FlashDataCommand : public Esp32Cli::Command {
public:
    explicit FlashDataCommand(Esp32DeltaOta& ota) : m_ota{ota} {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Esp32Cli::Client>& client) const override {
        if (argv.size() != 3) {
            Esp32Cli::Cli::printUsage(io, commandName, *this);;
            return;
        }

        const int compressedSize = strtol(argv[1].c_str(), nullptr, 0);
        const int dataSize = strtol(argv[2].c_str(), nullptr, 0);

        int ret;
        constexpr int BUFFER_SIZE = 1024;
        uint8_t buffer[BUFFER_SIZE];
        ota_state_t otaState;

        otaState.patchStream = &io;
        otaState.decompressor = heatshrink_decoder_alloc(32, 11, 9);
        otaState.compressedPatchSizeLeft = compressedSize;
        otaState.patchSize = dataSize;
        otaState.bytesWritten = 0;

        bspatch_stream patchStream{&otaState, readPatch};

        otaState.destPartition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "data");
        if (otaState.destPartition == nullptr) {
            io.println(R"({"error": "could not find data partition"})");
            return;
        }

        if(esp_littlefs_mounted("data")){
            esp_err_t err;
            int attempts = 4;
            while ((err = esp_vfs_littlefs_unregister("data")) != 0 && attempts-- > 0) {
                delay(500);
            }
            if(err){
                io.printf(R"({"error": "failed to unmount filesystem: %i"})""\n", err);
                while (patchStream.read(&patchStream, buffer, 1) == 0) {
                }
                return;
            }
        }

        ret = esp_partition_erase_range(otaState.destPartition, 0, dataSize);
        if (ret != ESP_OK) {
            io.printf(R"({"error": "esp partition erase failed with %i"})""\n", ret);
            while (patchStream.read(&patchStream, buffer, 1) == 0) {
            }
            return;
        }

        do {
            int readSize = std::min(BUFFER_SIZE, dataSize);
            ret = patchStream.read(&patchStream, buffer, readSize);
            if (ret != 0) {
                io.printf(R"({"error": "failed to read data: %i"})""\n", ret);
                ret = ESP_FAIL;
                break;
            }
            ret = esp_partition_write(otaState.destPartition, otaState.bytesWritten, buffer, readSize);
            if (ret != ESP_OK) {
                io.printf(R"({"error": "flashing failed with %i"})""\n", ret);
                break;
            }
            otaState.bytesWritten += readSize;
            if (otaState.bytesWritten >= otaState.nextStatusUpdate && io.availableForWrite() >= 24) {
                io.printf(R"({"progress": %i})""\n", otaState.bytesWritten);
                otaState.nextStatusUpdate += 16384;
            }
        } while (otaState.bytesWritten < dataSize);

        if (ret != ESP_OK) {
            while (patchStream.read(&patchStream, buffer, 1) == 0) {
            }
            return;
        }

        io.println(R"({"success": 1})");
    }

    void printUsage(Print& output) const override {
        output.println("<compressed_size> <data_size>");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        Esp32Cli::Cli::printUsage(output, commandName, *this);
        output.println("Update the data partition. ESP doesn't automatically restart after the update.");
    }

private:
    Esp32DeltaOta& m_ota;
};

class DumpCommand : public Esp32Cli::Command {
public:
    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Esp32Cli::Client>& client) const override {
        auto partition = esp_ota_get_boot_partition();
        esp_partition_pos_t partitionPosition = {partition->address, partition->size};
        esp_image_metadata_t imageMetadata;
        esp_image_get_metadata(&partitionPosition, &imageMetadata);

        heatshrink_encoder* encoder = heatshrink_encoder_alloc(11, 9);
        int heatshrinkResult = -1;

        constexpr uint32_t READ_BUF_SIZE = 1024;
        std::unique_ptr<uint8_t> readBuffer(new uint8_t[READ_BUF_SIZE]);
        std::unique_ptr<uint8_t> compressBuffer(new uint8_t[READ_BUF_SIZE]);

        for (uint32_t offset = 0; offset < imageMetadata.image_len; offset += READ_BUF_SIZE) {
            uint32_t readSize = std::min(READ_BUF_SIZE, imageMetadata.image_len - offset);
            size_t consumedBytes = 0;
            esp_partition_read(partition, offset, readBuffer.get(), readSize);
            do {
                size_t consumedSize = 0;
                size_t compressedSize = 0;

                heatshrinkResult = heatshrink_encoder_sink(encoder, readBuffer.get() + consumedBytes,
                                                           readSize - consumedBytes, &consumedSize);
                if (heatshrinkResult != HSER_SINK_OK) {
                    log_e("Heatshrink error %i in sink", heatshrinkResult);
                    ESP.restart();
                }
                consumedBytes += consumedSize;

                do {
                    heatshrinkResult = heatshrink_encoder_poll(encoder, compressBuffer.get(), READ_BUF_SIZE,
                                                               &compressedSize);
                    if (heatshrinkResult < 0) {
                        log_e("Heatshrink error %i in poll", heatshrinkResult);
                        ESP.restart();
                    }
                    io.write(compressBuffer.get(), compressedSize);
                } while (heatshrinkResult != HSER_POLL_EMPTY);
            } while (consumedBytes < readSize);
        }

        heatshrinkResult = heatshrink_encoder_finish(encoder);
        while (heatshrinkResult == HSER_FINISH_MORE) {
            size_t compressedSize = 0;
            do {
                heatshrinkResult = heatshrink_encoder_poll(encoder, compressBuffer.get(), READ_BUF_SIZE,
                                                           &compressedSize);
                if (heatshrinkResult < 0) {
                    log_e("Heatshrink error %i in poll", heatshrinkResult);
                    ESP.restart();
                }
                io.write(compressBuffer.get(), compressedSize);
            } while (heatshrinkResult != HSER_POLL_EMPTY);
            heatshrinkResult = heatshrink_encoder_finish(encoder);
        }
        if (heatshrinkResult != HSER_FINISH_DONE) {
            log_e("Heatshrink error %i in finish", heatshrinkResult);
            ESP.restart();
        }

        heatshrink_encoder_free(encoder);
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println("Dump the current running firmware as binary data.");
    }
};

class SelfTestCommand : public Esp32Cli::Command {
public:
    explicit SelfTestCommand(Esp32DeltaOta& ota) : m_ota{ota} {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Esp32Cli::Client>& client) const override {
        delay(1000); // Just a basic test to see if the Firmware even survives a second of runtime.
        if (ESP.getFreeHeap() < 40000) {
            io.println(R"({"error": "Recovery has too little free heap"})");
            Esp32DeltaOta::resetEsp();
        }
        m_ota.markProgramAsStable();
        io.println(R"({"success": 1})");
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println("Run a small self test.");
    }

private:
    Esp32DeltaOta& m_ota;
};

class RequestRecoveryCommand : public Esp32Cli::Command {
public:
    explicit RequestRecoveryCommand(Esp32DeltaOta& ota) : m_ota{ota} {}

    void execute(Stream& io, const std::string& commandName, std::vector<std::string>& argv, const std::shared_ptr<Esp32Cli::Client>& client) const override {
        m_ota.requestRecovery();
    }

    void printHelp(Print& output, const std::string& commandName, std::vector<std::string>& argv) const override {
        output.println("Run a small self test.");
    }

private:
    Esp32DeltaOta& m_ota;
};

FirmwareCommand::FirmwareCommand(Esp32DeltaOta& ota, bool doingRecoveryTest) {
    addCommand<InfoCommand>("info", ota);
    addCommand<DumpCommand>("dump");
    addCommand<FlashDeltaCommand>("flash-delta", ota);
    addCommand<FlashDataCommand>("flash-data", ota);
    addCommand<RequestRecoveryCommand>("request-recovery", ota);
    if (doingRecoveryTest) {
        addCommand<SelfTestCommand>("self-test", ota);
    }
}
};
