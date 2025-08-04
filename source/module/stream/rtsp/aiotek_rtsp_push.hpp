#ifndef __AIOTEK_RTSP_PUSH_HPP__
#define __AIOTEK_RTSP_PUSH_HPP__
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/error.h>

#ifdef __cplusplus
}
#endif

namespace aiotek {
namespace stream {
namespace rtsp {
class RTSPPushStream {
  private:
    /**
     * @brief get pkg rtsp input and convert to output
     *
     */
    void getStream();
    /**
     * @brief Thread handler get and push stream
     *
     */
    void threadPushStreamToServer();

  public:
    RTSPPushStream(const std::string& input_url, const std::string& output_url);
    ~RTSPPushStream();
    /**
     * @brief init RTSP stream
     *
     */
    void init();
    /**
     * @brief deinit RTSP
     *
     */
    void deinit();
    /**
     * @brief start get stream from RTSP of camera and push stream to server
     *
     */
    void start();
    /**
     * @brief stop get and push stream to server
     *
     */
    void stop();
    /**
     * @brief check if stream is running
     * 
     * @return true - is running
     * @return false - otherwise
     */
    bool isRunning() const;

  private:
    std::string m_input_rtsp_url;
    std::string m_output_rtsp_url;
    std::thread m_push_stream_thread;
    std::atomic<bool> m_is_running;
    std::mutex m_push_stream_mutex;
};
} // namespace rtsp
} // namespace stream
} // namespace aiotek

#endif /* __AIOTEK_RTSP_PUSH_HPP__ */