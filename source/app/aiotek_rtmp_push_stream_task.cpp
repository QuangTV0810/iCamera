#include "aiotek_task.hpp"
#include "aiotek_mailbox.hpp"
#include "aiotek_rtmp_push_stream_task.hpp"

namespace AIOTEK {
namespace app {

RTMPPushTask::RTMPPushTask(int id) : Task("RTMP-PUSH", id), m_is_thread_need_stop(false), m_is_thread_running(false), m_rtmp_pusher(nullptr)
{
    m_rtmp_pusher =
        std::make_unique<aiotek::module::stream::rtmp::RTMPPushStream>(
            "rtsp://192.168.137.181:554/live/0", 
            "rtmp://192.168.137.83:1935/live/stream"
        );
}

RTMPPushTask::~RTMPPushTask()
{
    stop();
    deinit();
}

void RTMPPushTask::init()
{
    if (nullptr != m_rtmp_pusher) {
        m_rtmp_pusher->init();
    } else {
        AIOTEK_LOG_ERROR("RTMPPushTask: RTMPPusher is nullptr");
    }
}

void RTMPPushTask::deinit()
{
    if (nullptr != m_rtmp_pusher) {
        m_rtmp_pusher->deinit();
    } else {
        AIOTEK_LOG_ERROR("RTMPPushTask: RTMPPusher is nullptr");
    }
}

void RTMPPushTask::start()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_is_thread_running.load()) {
        return;
    }

    if (nullptr == m_rtmp_pusher) {
        AIOTEK_LOG_ERROR("RTMPPushTask: RTMPPusher is nullptr");
        return;
    }

    m_is_thread_need_stop.store(false);
    // Start worker thread first, then flip running flag to reduce race windows
    m_thread = std::thread([this]() -> void { this->threadFunc(); });
    m_is_thread_running.store(true);
    // Start the underlying pusher once the worker lifecycle has begun
    m_rtmp_pusher->start();
}

void RTMPPushTask::stop()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_is_thread_running.load()) {
        return;
    }

    // Request worker to stop and wake it up via mailbox
    m_is_thread_need_stop.store(true);
    AIOTEK::g_mailbox.send({AIOTEK::TaskID::PUSH_RTMP_TASK_ID,
                            AIOTEK::TaskID::PUSH_RTMP_TASK_ID,
                            MailboxMessage{static_cast<int32_t>(RTMPPushSignal::TERMINATE_THREAD), "", 0}});

    if (m_thread.joinable()) {
        m_thread.join();
    }

    m_is_thread_running.store(false);

    // Stop underlying pusher after worker is down
    if (m_rtmp_pusher) {
        m_rtmp_pusher->stop();
    }
}

bool RTMPPushTask::state() const
{
    return m_is_thread_running.load();
}

void RTMPPushTask::threadFunc()
{
    while (false == m_is_thread_need_stop.load()) {
        AIOTEK::MailboxPacket packet = AIOTEK::g_mailbox.receive();
        if (packet.receiver == AIOTEK::TaskID::PUSH_RTMP_TASK_ID) {
            std::cout << "[RTMPPushTask] From: " << AIOTEK::TaskIDToString(packet.sender) << " To: " << AIOTEK::TaskIDToString(packet.receiver)
                      << std::endl;
            switch (packet.msg.signal) {
                case static_cast<int32_t>(RTMPPushSignal::TERMINATE_THREAD):
                    AIOTEK_LOG_INFO("RTMPPushTask: Received terminate signal, stopping thread");
                    // Honor terminate by setting stop flag; controller thread owns running state
                    m_is_thread_need_stop.store(true);
                    break;
                default:
                    AIOTEK_LOG_DEBUG("RTMPPushTask: Unknown signal: " + std::to_string(packet.msg.signal));
                    break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

} // namespace app

} // namespace AIOTEK
