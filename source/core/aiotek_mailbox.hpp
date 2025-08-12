#pragma once
#include <variant>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include "aiotek_task.hpp"

namespace AIOTEK {

struct MailboxMessage {
    std::int32_t signal;
    std::string msg;
    std::uint64_t len;
};

struct MailboxPacket {
    TaskID sender;
    TaskID receiver;
    MailboxMessage msg;
};

class Mailbox {
  public:
    void send(const MailboxPacket& env);
    MailboxPacket receive();
    std::optional<MailboxPacket> try_receive();

  private:
    std::queue<MailboxPacket> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};

extern Mailbox g_mailbox;
} // namespace AIOTEK
