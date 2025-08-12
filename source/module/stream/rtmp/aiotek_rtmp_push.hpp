// RTMPPushStream.hpp
#ifndef __AIOTEK_RTMP_PUSH_HPP__
#define __AIOTEK_RTMP_PUSH_HPP__

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/error.h>
}

namespace aiotek {
namespace module {
namespace stream {
namespace rtmp {

class RTMPPushStream {
public:
    RTMPPushStream(const std::string& input_url,
                   const std::string& output_url);

    ~RTMPPushStream();

    // Prepare FFmpeg network and contexts
    void init();

    // Teardown FFmpeg network
    void deinit();

    // Launch the background push thread
    void start();

    // Stop and join the thread
    void stop();

    // Is the stream currently running?
    bool isOperation() const;

private:
    // Set up input/output contexts and write RTMP header
    void setupStream();

    // The worker function that loops and pushes packets
    void threadPushStream();

private:
    std::string           m_input_rtsp_url;
    std::string           m_output_rtmp_url;

    // FFmpeg contexts
    AVFormatContext*      m_ifmt_ctx   = nullptr;
    AVFormatContext*      m_ofmt_ctx   = nullptr;

    // Threading
    std::thread           m_push_stream_thread;
    std::atomic<bool>     m_thread_need_stop{false};
    std::mutex            m_push_stream_mutex;
    std::atomic<bool>     m_is_stream_running{false};
};

} // namespace rtmp
} // namespace stream
} // namespace module
} // namespace aiotek

#endif /* __AIOTEK_RTMP_PUSH_HPP__ */
