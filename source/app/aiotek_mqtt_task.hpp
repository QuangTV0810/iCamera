#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include "aiotek_task.hpp"
#include "aiotek_mqtt.hpp"

namespace AIOTEK {

class MQTTTask : public Task {
public:
    MQTTTask(int id);
    ~MQTTTask();
    void start() override;
    void stop() override;
    bool state() const override;

private:
    void threadFunc();
    std::unique_ptr<MQTTManager> m_client; 
};

} // namespace AIOTEK 