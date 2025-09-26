#pragma once
#include <string>
#include <cstdio>
#include "aiotek_task.hpp"

extern "C" {
#include "aiotek_video.h"
}

namespace aiotek {
namespace app {

class VideoTask {
  public:
    VideoTask();
    ~VideoTask();

    void Initialize();
    void Deinitialize();
    void Start();
    void Stop();

    bool IsOperation();
    void ThreadVideoHandler(aiotek::core::Task& task);

  private:
    VideoCapture_t *m_video_handler;
    FILE *m_output_file;
};

} // namespace app
} // namespace aiotek