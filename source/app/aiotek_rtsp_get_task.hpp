#pragma once
#include <string>
#include "aiotek_task.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/error.h>
}
#include "aiotek_logger.hpp"

namespace aiotek {
namespace app {

class GetRTSPTask {
  public:
    GetRTSPTask(const std::string& rtsp_url);
    ~GetRTSPTask();

    void Initialize();
    void Deinitialize();
    void Start();
    void Stop();

    bool IsOperation();
    void ThreadGetRTSPHandler(aiotek::core::Task& task);

  private:
    std::string m_rtsp_url;
    AVFormatContext* m_ifmt_ctx;
};

} // namespace app
} // namespace aiotek