#ifndef __AIOTEK_DEF_H__
#define __AIOTEK_DEF_H__

namespace aiotek {
namespace common {
    
enum class MQTTSignal : int { 
    MQTT_CONNECT_SIG, 
    MQTT_DISCONNECT_SIG 
};

enum class RTSPSignal : int {
    RTSP_INIT_SUCCUSS_SIG = 1,
};
enum class RTMPSignal : int32_t {
    RTSP_INIT_SUCCUSS_SIG = 1,
    RTMP_START_PUSH_SIG,
    RTMP_STOP_PUSH_SIG
};
} // namespace common
} // namespace aiotek

#endif /* __AIOTEK_DEF_H__ */