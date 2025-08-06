#include <iostream>
#include <chrono>
#include "aiotek_mailbox.hpp"

#include "aiotek_push_stream_task.hpp"

namespace AIOTEK {
PushStreamTask::PushStreamTask(int id) : Task("PUSH_TASK_ID", id), m_is_thread_running(false), m_rtsp_pusher(nullptr), m_last_restart_time(std::chrono::steady_clock::now())
{
    m_rtsp_pusher = std::make_unique<aiotek::stream::rtsp::RTSPPushStream>(
        "rtsp://192.168.137.8:554/live/0", 
        "rtsp://192.168.137.71:554/live/stream"
    );
}
PushStreamTask::~PushStreamTask()
{
    stop();
}

void PushStreamTask::init()
{
    if (m_rtsp_pusher) {
        m_rtsp_pusher->init();
    }
}

void PushStreamTask::deinit() {

}

void PushStreamTask::start()
{
    if (m_rtsp_pusher) {
        m_rtsp_pusher->start();
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_is_thread_running)
        return;

    m_is_thread_running = true;
    m_thread = std::thread(&PushStreamTask::threadFunc, this);
    AIOTEK_LOG_INFO("PushStreamTask: Started");
}

void PushStreamTask::stop()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_is_thread_running)
        return;
    
    AIOTEK::g_mailbox.send({
        AIOTEK::TaskID::PUSH_STREAM_TASK_ID, 
        AIOTEK::TaskID::PUSH_STREAM_TASK_ID, 
        MailboxMessage{static_cast<int32_t>(PushStreamSignal::TERMINATE_THREAD), "TERMINATE", 9}
    });
    AIOTEK_LOG_INFO("PushStreamTask: Terminate signal sent");
    
    if (m_thread.joinable())
        m_thread.join();
    
    if (m_rtsp_pusher) {
        m_rtsp_pusher->stop();
        m_rtsp_pusher->deinit();
    }

    m_is_thread_running = false;

    AIOTEK_LOG_INFO("PushStreamTask: Stopped");
}

bool PushStreamTask::state() const
{
    return m_is_thread_running;
}

void PushStreamTask::threadFunc()
{
    AIOTEK_LOG_INFO("PushStreamTask: Thread running");

    while (m_is_thread_running) {

        auto packet_opt = AIOTEK::g_mailbox.try_receive();
        if (packet_opt.has_value()) {
            AIOTEK::MailboxPacket packet = packet_opt.value();
            if (packet.receiver == AIOTEK::TaskID::PUSH_STREAM_TASK_ID) {
                std::cout << "[PushStreamTask] From: " << AIOTEK::TaskIDToString(packet.sender) << " To: " << AIOTEK::TaskIDToString(packet.receiver)
                          << std::endl;
                switch (packet.msg.signal) {
                    case static_cast<int32_t>(PushStreamSignal::TERMINATE_THREAD):
                        AIOTEK_LOG_INFO("PushStreamTask: Received terminate signal, stopping thread");
                        m_is_thread_running = false;
                        break;
                    default:
                        AIOTEK_LOG_DEBUG("PushStreamTask: Unknown signal: " + std::to_string(packet.msg.signal));
                        break;
                }
            }
        }

        if (m_rtsp_pusher) {
            bool is_stream_running = m_rtsp_pusher->isRunning();
            if (!is_stream_running) {
                auto now = std::chrono::steady_clock::now();
                auto time_since_last_restart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - m_last_restart_time).count();
                
                if (time_since_last_restart > RESTART_COOLDOWN_MS) {
                    AIOTEK_LOG_WARNING("PushStreamTask: RTSP stream is not running, attempting restart...");
                } else {
                    AIOTEK_LOG_DEBUG("PushStreamTask: RTSP stream not running, but restart cooldown active (" 
                                   + std::to_string(time_since_last_restart) + "ms ago)");
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    AIOTEK_LOG_INFO("PushStreamTask: Thread exiting");
}

} // namespace AIOTEK
