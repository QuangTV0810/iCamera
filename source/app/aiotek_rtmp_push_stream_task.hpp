#ifndef __AIOTEK_RTMP_PUSH_H__
#define __AIOTEK_RTMP_PUSH_H__
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include "aiotek_task.hpp"
#include "aiotek_rtsp_push.hpp"
#include "aiotek_rtmp_push.hpp"

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
namespace app {
enum class RTMPPushSignal : int32_t {
    TERMINATE_THREAD = -1,
    START_PUSH_STREAM_SIG,
    STOP_PUSH_STREAM_SIG,
    PAUSE_PUSH_STREAM_SIG,
    CONTINUE_PUSH_STREAM_SIG
};

class RTMPPushTask : public Task {
  public:
    RTMPPushTask(int id);
    ~RTMPPushTask();
    void init() override;
    void deinit() override;
    void start() override;
    void stop() override;
    bool state() const override;

  private:
    void threadFunc();
    std::mutex m_mutex;
    std::atomic<bool> m_is_thread_need_stop;
    std::atomic<bool> m_is_thread_running{false};
    std::unique_ptr<aiotek::module::stream::rtmp::RTMPPushStream> m_rtmp_pusher{nullptr};
};
} // namespace app
} // namespace AIOTEK
#endif /* __AIOTEK_RTMP_PUSH_H__ */