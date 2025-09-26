#pragma once

namespace aiotek {
namespace app {
enum TaskID : int {
    TIMER_TASK,
    VIDEO_TASK,
    MQTT_TASK,
    RTSP_TASK,
    RTMP_TASK,
    CONSOLE_TASK,
    MAX_TASK,
};

void RegisterAllTask();
} // namespace app
} // namespace aiotek