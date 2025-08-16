#include "aiotek_ring_buffer.hpp"
#include <cstring>

namespace aiotek {
namespace core {

RingBuffer::RingBuffer(size_t max_size) : m_max_size(max_size)
{
}

RingBuffer::~RingBuffer()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pkt : m_buffer) {
        av_packet_unref(&pkt);
    }
    m_buffer.clear();
}

bool RingBuffer::push(const AVPacket& packet)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_buffer.size() >= m_max_size) {
        AVPacket& old = m_buffer.front();
        av_packet_unref(&old);
        m_buffer.pop_front();
    }

    m_buffer.emplace_back();
    AVPacket& dst = m_buffer.back();
    std::memset(&dst, 0, sizeof(dst));

    if (av_packet_ref(&dst, &packet) < 0) {
        m_buffer.pop_back();
        return false;
    }

    m_cond_push.notify_one();
    return true;
}

std::optional<AVPacket> RingBuffer::pop(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_cond_push.wait_for(lock, timeout, [this] { return !m_buffer.empty(); })) {
        return std::nullopt;
    }

    AVPacket out;
    std::memset(&out, 0, sizeof(out));
    av_packet_move_ref(&out, &m_buffer.front());
    m_buffer.pop_front();
    return out;
}

std::optional<AVPacket> RingBuffer::try_pop()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_buffer.empty())
        return std::nullopt;

    AVPacket out;
    std::memset(&out, 0, sizeof(out));
    av_packet_move_ref(&out, &m_buffer.front());
    m_buffer.pop_front();
    return out;
}

RingBufferManager& RingBufferManager::GetInstance()
{
    static RingBufferManager instance;
    return instance;
}

void RingBufferManager::CreateRingBuffer(const std::string& name, size_t max_size)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_ring_buffer.find(name) == m_ring_buffer.end()) {
        m_ring_buffer[name] = std::make_shared<RingBuffer>(max_size);
    }
}

void RingBufferManager::ReleaseRingBuffer(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ring_buffer.erase(name);
}

std::shared_ptr<RingBuffer> RingBufferManager::GetRingBuffer(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ring_buffer.find(name);
    if (it != m_ring_buffer.end()) {
        return it->second;
    }
    return nullptr;
}
} // namespace core
} // namespace aiotek
