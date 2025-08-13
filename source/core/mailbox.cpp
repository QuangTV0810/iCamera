#include "mailbox.hpp"

namespace aiotek {
namespace core {
void Mailbox::send(const MailboxPacket& env)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push_back(env);
    m_cond.notify_one();
}

MailboxPacket Mailbox::receive(TaskID receiver_id)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock, [this, receiver_id] {
        for (auto it = m_queue.begin(); it != m_queue.end(); ++it) {
            if (it->receiver == receiver_id)
                return true;
        }
        return false;
    });
    // Find first packet for receiver_id
    for (auto it = m_queue.begin(); it != m_queue.end(); ++it) {
        if (it->receiver == receiver_id) {
            MailboxPacket env = *it;
            m_queue.erase(it);
            return env;
        }
    }
    // Should not reach here due to wait condition
    return MailboxPacket{0, 0, {0, "", 0}}; // Fallback
}

std::optional<MailboxPacket> Mailbox::receive(TaskID receiver_id, std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_cond.wait_for(lock, timeout, [this, receiver_id] {
            for (auto it = m_queue.begin(); it != m_queue.end(); ++it) {
                if (it->receiver == receiver_id)
                    return true;
            }
            return false;
        })) {
        return std::nullopt; // Timeout
    }
    // Find first packet for receiver_id
    for (auto it = m_queue.begin(); it != m_queue.end(); ++it) {
        if (it->receiver == receiver_id) {
            MailboxPacket env = *it;
            m_queue.erase(it);
            return env;
        }
    }
    return std::nullopt; // Should not reach here
}

std::optional<MailboxPacket> Mailbox::try_receive(TaskID receiver_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_queue.begin(); it != m_queue.end(); ++it) {
        if (it->receiver == receiver_id) {
            MailboxPacket env = *it;
            m_queue.erase(it);
            return env;
        }
    }
    return std::nullopt;
}

Mailbox m_mailbox;
} // namespace core

} // namespace aiotek