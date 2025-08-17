#pragma once
#include <deque>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>
#include <map>
#include <memory>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/error.h>
}

namespace aiotek {
namespace core {

class RingBuffer {
  public:
    explicit RingBuffer(size_t max_size = 100);
    ~RingBuffer();

    bool push(const AVPacket& packet);

    std::optional<AVPacket> pop(std::chrono::milliseconds timeout = std::chrono::milliseconds(100));
    std::optional<AVPacket> try_pop();

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

  private:
    std::deque<AVPacket> m_buffer;
    std::mutex m_mutex;
    std::condition_variable m_cond_push;
    size_t m_max_size;
};
class RingBufferManager {
  private:
    RingBufferManager() = default;
    ~RingBufferManager() = default;

    RingBufferManager(const RingBufferManager&) = delete;
    RingBufferManager& operator=(const RingBufferManager&) = delete;

  public:
    static RingBufferManager& GetInstance();
    void CreateRingBuffer(const std::string& name, size_t max_size);
    void ReleaseRingBuffer(const std::string& name);
    std::shared_ptr<RingBuffer> GetRingBuffer(const std::string& name);

  private:
    std::map<std::string, std::shared_ptr<RingBuffer>> m_ring_buffer;
    std::mutex m_mutex;
};

} // namespace core
} // namespace aiotek
