#ifndef __AIOTEK_PUSH_STREAM_H__
#define __AIOTEK_PUSH_STREAM_H__
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include "aiotek_task.hpp"
#include "aiotek_rtsp_push.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/error.h>

#ifdef __cplusplus
}
#endif

namespace AIOTEK {

class PushStreamTask : public Task {
public:
    PushStreamTask(int id);
    ~PushStreamTask();
    void init() override;
    void start() override;
    void stop() override;
    bool state() const override;

private:
    void threadFunc();
    aiotek::stream::rtsp::RTSPPushStream m_rtsp_pusher;
};

} // namespace AIOTEK 

#endif /* __AIOTEK_PUSH_STREAM_H__ */