#pragma once

#include <variant>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <any>

namespace aiotek {
namespace core {

using TaskID = int;

struct MailboxPacket {
    TaskID task_sender_id;
    TaskID task_receiver_id;
    std::int32_t signal;
    std::any msg;
    std::uint64_t len;
};

class Mailbox {
public:
    explicit Mailbox(size_t max_queue_size = 100, TaskID task_id = 0);
    bool send(const MailboxPacket& packet);
    MailboxPacket receive();
    std::optional<MailboxPacket> receive(std::chrono::milliseconds timeout);
private:
    std::deque<MailboxPacket> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    size_t m_max_queue_size;
    TaskID m_task_id;
};

} // namespace core
} // namespace aiotek