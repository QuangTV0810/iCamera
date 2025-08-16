#pragma once

namespace aiotek {
namespace app {
enum TaskID : int {
    MQTT_TASK,
    RTSP_TASK,
    RTMP_TASK,
    MAX_TASK,
};
void RegisterAllTask();
} // namespace app
} // namespace aiotek