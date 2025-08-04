#include <iostream>
#include <chrono>
#include "aiotek_mailbox.hpp"

#include "aiotek_push_stream_task.hpp"

namespace AIOTEK {
PushStreamTask::PushStreamTask(int id) : 
    Task("PUSH_TASK_ID", id), 
    m_rtsp_pusher("rtsp://192.168.137.96:554/live/0", "rtsp://192.168.137.71:554/live/stream")
{
}
PushStreamTask::~PushStreamTask()
{
    stop();
}

void PushStreamTask::init()
{
    m_rtsp_pusher.init();
}

void PushStreamTask::start()
{
    m_rtsp_pusher.start();

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_running)
        return;

    m_running = true;
    m_thread = std::thread(&PushStreamTask::threadFunc, this);
    AIOTEK_LOG_INFO("PushStreamTask: Started");
}

void PushStreamTask::stop()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running)
        return;
    m_running = false;
    if (m_thread.joinable())
        m_thread.join();

    m_rtsp_pusher.stop();
    m_rtsp_pusher.deinit();

    AIOTEK_LOG_INFO("PushStreamTask: Stopped");
}

bool PushStreamTask::state() const
{
    return m_running;
}

void PushStreamTask::threadFunc()
{
    AIOTEK_LOG_INFO("PushStreamTask: Thread running");

    while (m_running) {
        AIOTEK::MailboxPacket packet = AIOTEK::g_mailbox.receive();
        if (packet.receiver == AIOTEK::TaskID::PUSH_STREAM_TASK_ID) {
            std::cout << "[MQTTTask] From: " << AIOTEK::TaskIDToString(packet.sender) << " To: " << AIOTEK::TaskIDToString(packet.receiver)
                      << std::endl;
            switch (packet.msg.signal) {
                case 1:
                    if (!packet.msg.msg.empty())
                        std::cout << "Msg: " << packet.msg.msg << std::endl;
                    break;
                default:
                    break;
            }
        }
    }

    AIOTEK_LOG_INFO("PushStreamTask: Thread exiting");
}

} // namespace AIOTEK
