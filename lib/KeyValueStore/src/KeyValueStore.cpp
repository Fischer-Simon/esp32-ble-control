#include "KeyValueStore.h"

#include <LightweightMap.h>
#include <nvs.h>
#include <Arduino.h>

KeyValueStore::KeyValueStore() {
    m_valuePersistenceTimer = xTimerCreate("ValuePersistenceTimer", pdMS_TO_TICKS(1000), pdFALSE, this,
                                           &valuePersistenceTimerFn);
}

KeyValueStore::~KeyValueStore() {
    std::unique_lock<std::mutex> lock(m_mutex);
    xTimerDelete(m_valuePersistenceTimer, portMAX_DELAY);
    for (const auto& value: m_values) {
        value->clearStore();
    }
}

void KeyValueStore::notifyValueChange(Value& value) {
    auto lock = acquireLock();
    if (value.m_store != this) {
        return;
    }
    auto valuePtr = value.shared_from_this();
    for (const auto& cb: m_valueChangeCallbacks) {
        cb.second(valuePtr);
    }
    if (valuePtr->m_persistenceStatus == Value::PersistedClean) {
        valuePtr->m_persistenceStatus = Value::PersistedModified;
        m_changedPersistentValues.push_back(std::move(valuePtr));
        xTimerStart(m_valuePersistenceTimer, portMAX_DELAY);
    }
}

void KeyValueStore::writeAllValuesToStream(Stream& stream) const {
    constexpr size_t bufferSize = 128;
    uint8_t valueBuffer[bufferSize];
    auto lock = acquireLock();
    for (const auto& value : m_values) {
        size_t writeSize = value->writeToBuffer(valueBuffer, bufferSize);
        stream.write(valueBuffer, writeSize);
    }
}

void KeyValueStore::loadValueFromPersistence(const char* namespace_, const char* key, uint8_t* data, size_t dataSize) {
    nvs_handle_t nvsHandle;
    nvs_open(namespace_, NVS_READONLY, &nvsHandle);
    nvs_get_blob(nvsHandle, key, data, &dataSize);
    nvs_close(nvsHandle);
}

void KeyValueStore::valuePersistenceHandler() {
    auto lock = acquireLock();
    std::map<std::string, nvs_handle_t> openNvsHandles;
    for (auto& value : m_changedPersistentValues) {
        nvs_handle_t nvsHandle;
        auto nvsHandleItor = openNvsHandles.find(value->m_namespace);
        if (nvsHandleItor == openNvsHandles.end()) {
            nvs_open(value->m_namespace, NVS_READWRITE, &nvsHandle);
            openNvsHandles.emplace(value->m_namespace, nvsHandle);
        } else {
            nvsHandle = nvsHandleItor->second;
        }
        auto valueLock = value->acquireLock();
        nvs_set_blob(nvsHandle, value->m_key, value->data(), value->getValueSize());
    }
    for (auto nvsHandle : openNvsHandles) {
        nvs_commit(nvsHandle.second);
        nvs_close(nvsHandle.second);
    }
    m_changedPersistentValues.clear();
}

void KeyValueStore::valuePersistenceTimerFn(TimerHandle_t timer) {
    static_cast<KeyValueStore*>(pvTimerGetTimerID(timer))->valuePersistenceHandler();
}
