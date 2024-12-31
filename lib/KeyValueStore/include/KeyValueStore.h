#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <cstring>
#include <HWCDC.h>
#include <list>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <Stream.h>

class KeyValueStore {
public:
    class Value : public std::enable_shared_from_this<Value> {
        friend class KeyValueStore;

        struct Private {
        };

    public:
        enum PersistenceStatus {
            Volatile,
            PersistedClean,
            PersistedModified,
        };

        Value(KeyValueStore* store, const char* namespace_, const char* key, bool isPersistent, Private)
            : m_store{store},
              m_namespace{strdup(namespace_)},
              m_key{strdup(key)},
              m_persistenceStatus{isPersistent ? PersistedClean : Volatile} {
            size_t nameSize = strlen(m_namespace) + strlen(key) + 2;
            m_name = static_cast<char*>(malloc(nameSize));
            snprintf(m_name, nameSize, "%s/%s", m_namespace, m_key);
        }

        virtual ~Value() {
            free(m_name);
            free(m_key);
            free(m_namespace);
        }

        const char* name() const {
            return m_name;
        }

        size_t writeToBuffer(uint8_t* buffer, size_t bufferSize, size_t* requiredBufferSize = nullptr) const {
            auto lock = acquireLock();
            const size_t nameSize = strlen(m_name) + 1;
            size_t valueSize = getValueSize();
            assert(valueSize < 255);
            size_t capacityRequired = nameSize + 1 + valueSize;
            if (requiredBufferSize != nullptr) {
                *requiredBufferSize = capacityRequired;
            }
            if (bufferSize < capacityRequired) {
                return 0;
            }
            memcpy(buffer, m_name, nameSize - 1);
            buffer[nameSize - 1] = '\0';
            buffer[nameSize] = static_cast<uint8_t>(valueSize);
            if (writeValue(buffer + nameSize + 1, bufferSize - nameSize - 1) != valueSize) {
                return 0;
            }
            return capacityRequired;
        }

    protected:
        std::unique_lock<std::mutex> acquireLock() const {
            return std::unique_lock<std::mutex>(m_mutex);
        }

        virtual size_t getValueSize() const = 0;

        virtual size_t writeValue(uint8_t* buf, size_t bufSize) const = 0;

        virtual uint8_t* data() = 0;

        virtual const uint8_t* data() const = 0;

        void notifyChange() {
            if (!m_store) {
                return;
            }
            m_store->notifyValueChange(*this);
        }

    protected:
        void clearStore() {
            auto lock = acquireLock();
            m_store = nullptr;
        }

        mutable std::mutex m_mutex;
        KeyValueStore* m_store{nullptr};
        char* m_namespace;
        char* m_key;
        char* m_name;
        PersistenceStatus m_persistenceStatus;
    };

    template<typename T>
    class SimpleValue : public Value {
    public:
        static_assert(sizeof(T) < 255);

        SimpleValue(KeyValueStore* store, const char* namespace_, const char* key, bool isPersistent,
                    const T& initialValue, Private p)
            : Value{store, namespace_, key, isPersistent, p},
              m_value{initialValue} {
        }

        const T& value() const {
            return m_value;
        }

        void setValue(const T& value) {
            m_value = value;
            notifyChange();
        }

    protected:
        size_t getValueSize() const override {
            return sizeof(T);
        }

        size_t writeValue(uint8_t* buf, size_t bufSize) const override {
            if (bufSize < sizeof(T)) {
                return 0;
            }
            *reinterpret_cast<T*>(buf) = m_value;
            return sizeof(T);
        }

        uint8_t* data() override {
            return reinterpret_cast<uint8_t*>(&m_value);
        }

        const uint8_t* data() const override {
            return reinterpret_cast<const uint8_t*>(&m_value);
        }

    private:
        T m_value;
    };

    KeyValueStore();

    ~KeyValueStore();

    template<typename T>
    std::shared_ptr<SimpleValue<T> > createValue(const char* valueNamespace, const char* valueKey,
                                                 bool isPersistent, T defaultValue) {
        auto lock = acquireLock();
        if (isPersistent) {
            loadValueFromPersistence(valueNamespace, valueKey, reinterpret_cast<uint8_t*>(&defaultValue), sizeof(T));
        }
        auto value = std::make_shared<SimpleValue<T> >(
            this, valueNamespace, valueKey, isPersistent, defaultValue, Value::Private{}
        );
        m_values.emplace_back(value);
        return value;
    }

    void notifyValueChange(Value& value);

    void addValueChangeCallback(void* ownerPtr, std::function<void(const std::shared_ptr<const Value>&)> cb) {
        auto lock = acquireLock();
        m_valueChangeCallbacks[ownerPtr] = std::move(cb);
    }

    void removeValueChangeCallback(void* ownerPtr) {
        auto lock = acquireLock();
        m_valueChangeCallbacks.erase(ownerPtr);
    }

    void writeAllValuesToStream(Stream& stream) const;

private:
    std::unique_lock<std::mutex> acquireLock() const {
        return std::unique_lock<std::mutex>(m_mutex);
    }

    static void loadValueFromPersistence(const char* namespace_, const char* key, uint8_t* data, size_t dataSize);

    void valuePersistenceHandler();

    static void valuePersistenceTimerFn(TimerHandle_t timer);

    mutable std::mutex m_mutex;
    TimerHandle_t m_valuePersistenceTimer{nullptr};
    std::vector<std::shared_ptr<Value> > m_values;
    std::list<std::shared_ptr<const Value> > m_changedPersistentValues;
    std::map<void*, std::function<void(const std::shared_ptr<const Value>&)> > m_valueChangeCallbacks;
};
