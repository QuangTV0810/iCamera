#pragma once

#include <variant>
#include <string>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>
#include "task.hpp" // Thay aiotek_task.hpp bằng task.hpp từ code trước

namespace aiotek {
namespace core {
using TaskID = int; // Đồng bộ với Task::m_task_id

struct MailboxMessage {
    std::int32_t signal; // Signal code (e.g., command type)
    std::string msg;     // Message content
    std::uint64_t len;   // Length (optional, kept for compatibility)
};

struct MailboxPacket {
    TaskID sender;      // Sender task ID
    TaskID receiver;    // Receiver task ID
    MailboxMessage msg; // Message content
};

class Mailbox {
  public:
    /**
     * @brief Send a packet to the mailbox.
     * @param env Packet to send.
     */
    void send(const MailboxPacket& env);

    /**
     * @brief Receive a packet for a specific task (blocking).
     * @param receiver_id Task ID to filter packets.
     * @return Received packet.
     */
    MailboxPacket receive(TaskID receiver_id);

    /**
     * @brief Receive a packet with timeout for a specific task.
     * @param receiver_id Task ID to filter packets.
     * @param timeout Duration to wait before giving up.
     * @return Optional packet (std::nullopt if timed out or no packet).
     */
    std::optional<MailboxPacket> receive(TaskID receiver_id, std::chrono::milliseconds timeout);

    /**
     * @brief Try to receive a packet for a specific task (non-blocking).
     * @param receiver_id Task ID to filter packets.
     * @return Optional packet (std::nullopt if no packet).
     */
    std::optional<MailboxPacket> try_receive(TaskID receiver_id);

  private:
    std::deque<MailboxPacket> m_queue; // Container of packets (iterable/erasable)
    std::mutex m_mutex;                // Mutex for thread-safety
    std::condition_variable m_cond;    // Condition variable for blocking receive
};

extern Mailbox m_mailbox; // Global mailbox (consider instance-based for modularity)
} // namespace core

} // namespace aiotek