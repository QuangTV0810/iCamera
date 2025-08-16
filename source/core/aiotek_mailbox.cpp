#include "aiotek_mailbox.hpp"

namespace aiotek {
namespace core {

Mailbox::Mailbox(size_t max_queue_size, TaskID task_id) : m_max_queue_size(max_queue_size), m_task_id(task_id)
{
}

bool Mailbox::send(const MailboxPacket& packet)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (packet.task_receiver_id != m_task_id) {
        return false;
    }
    if (m_queue.size() >= m_max_queue_size) {
        m_queue.pop_front();
    }
    m_queue.push_back(packet);
    m_cond.notify_one();
    return true;
}

MailboxPacket Mailbox::receive()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock, [this] { return !m_queue.empty(); });
    MailboxPacket packet = m_queue.front();
    m_queue.pop_front();
    return packet;
}

std::optional<MailboxPacket> Mailbox::receive(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_cond.wait_for(lock, timeout, [this] { return !m_queue.empty(); })) {
        return std::nullopt;
    }
    MailboxPacket packet = m_queue.front();
    m_queue.pop_front();
    return packet;
}

} // namespace core
} // namespace aiotek