#include <iostream>
#include <chrono>
#include "aiotek_task.hpp"
#include "aiotek_mailbox.hpp"
#include "aiotek_mqtt.hpp"
#include "aiotek_net_managers.hpp"
#include "aiotek_mqtt_task.hpp"

namespace AIOTEK {

MQTTTask::MQTTTask(int id) : Task("MQTT", id), m_is_thread_running(false), m_client(nullptr)
{
}

MQTTTask::~MQTTTask()
{
    stop();
}

void MQTTTask::init()
{
}

void MQTTTask::deinit()
{
}

void MQTTTask::start()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_is_thread_running)
        return;

    m_client = std::make_unique<MQTTManager>();
    m_client->onConnect([&]() {
        MQTTManager::MQTTConfig config = m_client->getConfig();
        std::cout << "Connected to ThingBoard broker!" << std::endl;
        std::cout << "Device Token: " << config.username << std::endl;
        std::cout << "Broker: " << config.broker << ":" << config.port << " (SSL/TLS)" << std::endl;

        std::string msg = "hello";
        AIOTEK::g_mailbox.send({AIOTEK::TaskID::MQTT_TASK_ID, AIOTEK::TaskID::MQTT_TASK_ID, MailboxMessage{1, msg, msg.length()}});
    });

    m_client->onDisconnect([]() { std::cout << "Disconnected from ThingBoard broker!" << std::endl; });

    m_client->onMessage([](const std::string& topic, const nlohmann::json& data) {
        std::cout << "Received on topic '" << topic << "': " << data.dump(2) << std::endl;
    });

    m_client->onError([](const std::string& error) { std::cout << "Error: " << error << std::endl; });

    m_client->connect();

    m_is_thread_running = true;
    m_thread = std::thread(&MQTTTask::threadFunc, this);
    AIOTEK_LOG_INFO("MQTTTask: Started");
}

void MQTTTask::stop()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_is_thread_running)
        return;

    // Send terminate signal to this task via mailbox
    AIOTEK::g_mailbox.send({AIOTEK::TaskID::MQTT_TASK_ID, AIOTEK::TaskID::MQTT_TASK_ID,
                            MailboxMessage{static_cast<int32_t>(MQTTSignal::TERMINATE_THREAD), "TERMINATE", 9}});
    AIOTEK_LOG_INFO("MQTTTask: Terminate signal sent");

    // Wait for thread to finish
    if (m_thread.joinable())
        m_thread.join();

    m_is_thread_running = false;
    AIOTEK_LOG_INFO("MQTTTask: Stopped");
}

bool MQTTTask::state() const
{
    return m_is_thread_running;
}

void MQTTTask::threadFunc()
{
    AIOTEK_LOG_INFO("MQTTTask: Thread running");
    while (m_is_thread_running) {
        AIOTEK::MailboxPacket packet = AIOTEK::g_mailbox.receive();
        if (packet.receiver == AIOTEK::TaskID::MQTT_TASK_ID) {
            std::cout << "[MQTTTask] From: " << AIOTEK::TaskIDToString(packet.sender) << " To: " << AIOTEK::TaskIDToString(packet.receiver)
                      << std::endl;
            switch (packet.msg.signal) {
                case static_cast<int32_t>(MQTTSignal::CONNECT):
                    if (!packet.msg.msg.empty())
                        std::cout << "Msg: " << packet.msg.msg << std::endl;
                    break;
                case static_cast<int32_t>(MQTTSignal::TERMINATE_THREAD):
                    AIOTEK_LOG_INFO("MQTTTask: Received terminate signal, stopping thread");
                    m_is_thread_running = false;
                    break;
                default:
                    AIOTEK_LOG_DEBUG("MQTTTask: Unknown signal: " + std::to_string(packet.msg.signal));
                    break;
            }
        }
    }
    AIOTEK_LOG_INFO("MQTTTask: Thread exiting");
}

} // namespace AIOTEK