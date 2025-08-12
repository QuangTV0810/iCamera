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

enum class PushStreamSignal : int32_t {
    TERMINATE_THREAD = -1,
    START_PUSH_STREAM_SIG,
    STOP_PUSH_STREAM_SIG,
    PAUSE_PUSH_STREAM_SIG,
    CONTINUE_PUSH_STREAM_SIG
};

class PushStreamTask : public Task {
  public:
    PushStreamTask(int id);
    ~PushStreamTask();
    void init() override;
    void deinit() override;
    void start() override;
    void stop() override;
    bool state() const override;

  private:
    void threadFunc();
    std::mutex m_mutex;
    std::atomic<bool> m_is_thread_running{false};
    std::unique_ptr<aiotek::stream::rtsp::RTSPPushStream> m_rtsp_pusher{nullptr};
    std::chrono::steady_clock::time_point m_last_restart_time;
    static constexpr int RESTART_COOLDOWN_MS = 5000;
};

} // namespace AIOTEK

#endif /* __AIOTEK_PUSH_STREAM_H__ */