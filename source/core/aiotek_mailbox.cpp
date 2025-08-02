#include "aiotek_mailbox.hpp"

namespace AIOTEK {

void Mailbox::send(const MailboxPacket& env)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(env);
    m_cond.notify_one();
}

MailboxPacket Mailbox::receive()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock, [this] { return !m_queue.empty(); });
    auto env = m_queue.front();
    m_queue.pop();
    return env;
}

std::optional<MailboxPacket> Mailbox::try_receive()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.empty())
        return std::nullopt;
    auto env = m_queue.front();
    m_queue.pop();
    return env;
}

Mailbox g_mailbox;
} // namespace AIOTEK
