#ifndef _AIOTEK_RTMP_PUSH_TASK_HPP_
#define _AIOTEK_RTMP_PUSH_TASK_HPP_

#include <string>
#include <vector>
#include <memory>
#include "aiotek_ring_buffer.hpp"
#include "aiotek_task.hpp"
#include "aiotek_rtsp_get_task.hpp"

extern "C" {
#include <libavformat/avformat.h>
}

namespace aiotek {
namespace app {

class PushRTMPTask {
  public:
    PushRTMPTask(const std::string& rtmp_url);
    ~PushRTMPTask();

    void Initialize();
    void Deinitialize();
    void Start();
    void Stop();

    bool IsOperation();
    void ThreadPushRTMPHandler(aiotek::core::Task& task);

  private:
    std::string m_rtmp_url;
    AVFormatContext* m_ifmt_ctx;
    AVFormatContext* m_ofmt_ctx;
    std::vector<int64_t> m_prev_dts;
    std::atomic<bool> m_is_pushing;
};

} // namespace app
} // namespace aiotek

#endif // _AIOTEK_RTMP_PUSH_TASK_HPP_