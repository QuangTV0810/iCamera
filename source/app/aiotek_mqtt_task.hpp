#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include "aiotek_task.hpp"
#include "aiotek_mqtt.hpp"

namespace AIOTEK {

// MQTT Signal enum class to manage all MQTT task signals
enum class MQTTSignal : uint8_t {
    // Thread control signals
    TERMINATE_THREAD = 0,
    
    // MQTT operation signals
    CONNECT = 10,
    DISCONNECT = 11,
    PUBLISH = 12,
    SUBSCRIBE = 13,
    UNSUBSCRIBE = 14,
};

class MQTTTask : public Task {
public:
    MQTTTask(int id);
    ~MQTTTask();
    void init() override;
    void start() override;
    void stop() override;
    bool state() const override;

private:
    void threadFunc();
    std::atomic<bool> m_is_thread_running;
    std::unique_ptr<MQTTManager> m_client;
};

} // namespace AIOTEK 