#include "aiotek_task.hpp"
#include "aiotek_task_list.hpp"
#include "aiotek_rtsp_get_task.hpp"
#include "aiotek_rtmp_push_task.hpp"

namespace aiotek {
namespace app {

void RegisterAllTask()
{
    aiotek::core::TaskManager::RegisterTask(
        [](aiotek::core::Task& task) {
            AIOTEK_LOG_INFO("GetRTSPTask: Registering handler");
            try {
                aiotek::app::GetRTSPTask m_rtsp("rtsp://192.168.137.11:554/live/0");
                m_rtsp.ThreadGetRTSPHandler(task);
            } catch (const std::exception& e) {
                std::cout << "GetRTSPTask: Exception in handler: " << e.what() << std::endl;
            }
        },
        "RTSP_TASK", RTSP_TASK);

    aiotek::core::TaskManager::RegisterTask(
        [](aiotek::core::Task& task) {
            AIOTEK_LOG_INFO("PushRTMPTask: Registering handler");
            try {
                aiotek::app::PushRTMPTask m_rtmp("rtmp://192.168.137.45:1935/live/stream");
                m_rtmp.ThreadPushRTMPHandler(task);
            } catch (const std::exception& e) {
                std::cout << "PushRTMPTask: Exception in handler: " << e.what() << std::endl;
            }
        },
        "RTMP_TASK", RTMP_TASK);
}
} // namespace app
} // namespace aiotek