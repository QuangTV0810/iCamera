#include "aiotek_task.hpp"
#include "aiotek_task_list.hpp"
#include "aiotek_rtsp_get_task.hpp"
#include "aiotek_rtmp_push_task.hpp"
#include "aiotek_console_task.hpp"
#include "aiotek_timer.hpp"

namespace aiotek {
namespace app {

void RegisterAllTask()
{
        aiotek::core::TaskManager::RegisterTask(
        [](aiotek::core::Task& task) {
            try {
                aiotek::core::TimerManager& timer_manager = aiotek::core::TimerManager::GetInstance();
                timer_manager.ThreadTimerHandler(task);
            } catch (const std::exception& e) {
                std::cout << "TimerManager: Exception in handler: " << e.what() << std::endl;
            }
        },
        "TIMER_TASK", TIMER_TASK);

    aiotek::core::TaskManager::RegisterTask(
        [](aiotek::core::Task& task) {
            try {
                aiotek::app::GetRTSPTask rtsp_task("rtsp://192.168.137.64:554/live/0");
                rtsp_task.ThreadGetRTSPHandler(task);
            } catch (const std::exception& e) {
                std::cout << "GetRTSPTask: Exception in handler: " << e.what() << std::endl;
            }
        },
        "RTSP_TASK", RTSP_TASK);

    aiotek::core::TaskManager::RegisterTask(
        [](aiotek::core::Task& task) {
            try {
                aiotek::app::PushRTMPTask rtmp_task("rtmp://192.168.137.45:1935/live/stream");
                rtmp_task.ThreadPushRTMPHandler(task);
            } catch (const std::exception& e) {
                std::cout << "PushRTMPTask: Exception in handler: " << e.what() << std::endl;
            }
        },
        "RTMP_TASK", RTMP_TASK);

    aiotek::core::TaskManager::RegisterTask(
        [](aiotek::core::Task& task) {
            try {
                aiotek::app::ConsoleTask console_task;
                console_task.ThreadConsoleHandler(task);
            } catch (const std::exception& e) {
                AIOTEK_LOG_ERROR("ConsoleTask: Exception in handler: " << e.what());
            }
        },
        "CONSOLE_TASK", CONSOLE_TASK);
}
} // namespace app
} // namespace aiotek