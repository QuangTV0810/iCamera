#include "aiotek_video_task.hpp"
#include "aiotek_logger.hpp"

namespace aiotek {
namespace app {

VideoTask::VideoTask() : m_video_handler(nullptr)
{
}

VideoTask::~VideoTask()
{
    Deinitialize();
}

void VideoTask::Initialize()
{
    // Initialize video configuration
    VideoConfig_t config = {.width = 1920, .height = 1080, .codec = RK_VIDEO_ID_AVC};

    // Create and initialize VideoHandler
    m_video_handler = VideoCapture_Create(&config);
    if (!m_video_handler) {
        AIOTEK_LOG_ERROR("VideoTask: Failed to create VideoHandler");
        throw std::runtime_error("Failed to create VideoHandler");
    }
    // Open output file
    m_output_file = fopen("/tmp/output.h264", "a");
    if (!m_output_file) {
        AIOTEK_LOG_ERROR("VideoTask: Failed to open output file /tmp/output.h264");
        VideoCapture_Destroy(m_video_handler);
        throw std::runtime_error("Failed to open output file");
    } else {
        AIOTEK_LOG_INFO("Open file /tmp/output.h264 is success");
    }

    // Register callback to write frames to file
    VideoCapture_RegisterCallbackVideo(
        m_video_handler,
        [](void* pData, uint32_t len, uint64_t pts, void* user_data) {
            FILE* file = static_cast<FILE*>(user_data);
            size_t written = fwrite(pData, 1, len, file);
            if (written != len) {
                AIOTEK_LOG_ERROR("VideoTask: Failed to write failure");
            } else {
                // AIOTEK_LOG_DEBUG("VideoTask: Wrote " << len << " bytes pts=" << pts);
            }
        },
        m_output_file);

    VideoCapture_Initialize(m_video_handler);
}

void VideoTask::Deinitialize()
{
    this->Stop();

    if (m_video_handler) {
        VideoCapture_Deinitialize(m_video_handler);
        VideoCapture_Destroy(m_video_handler);
        m_video_handler = nullptr;
    }
    if (m_output_file) {
        fclose(m_output_file);
        m_output_file = nullptr;
        AIOTEK_LOG_INFO("VideoTask: Closed output file /tmp/output.h264");
    }
}

void VideoTask::Start()
{
    if (!m_video_handler) {
        AIOTEK_LOG_ERROR("VideoTask: VideoHandler not initialized");
        throw std::runtime_error("VideoHandler not initialized");
    }
    VideoCapture_Start(m_video_handler);
    AIOTEK_LOG_INFO("VideoTask: Started");
}

void VideoTask::Stop()
{
    AIOTEK_LOG_INFO("VideoTask: Stopping");
    if (m_video_handler) {
        VideoCapture_Stop(m_video_handler);
    }
    AIOTEK_LOG_INFO("VideoTask: Stopped");
}

bool VideoTask::IsOperation()
{
    return VideoCapture_IsOperation(m_video_handler);
}

void VideoTask::ThreadVideoHandler(aiotek::core::Task& task)
{
    this->Initialize();
    this->Start();

    while (task.IsOperation()) {
        if (task.IsSuspended()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30fps
    }
}

} // namespace app
} // namespace aiotek