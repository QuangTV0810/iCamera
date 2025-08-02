#pragma once
#include <variant>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <iostream>

namespace AIOTEK {

enum class TaskID {
    Unknown = 0,
    Console,
    Sender,
    Receiver,
    Audio,
    Video,
    Managers,
    MQTT,
};

inline const char* TaskIDToString(TaskID id)
{
    switch (id) {
        case TaskID::Unknown:
            return "Unknown";
        case TaskID::Console:
            return "Console";
        case TaskID::Sender:
            return "Sender";
        case TaskID::Receiver:
            return "Receiver";
        case TaskID::Audio:
            return "Audio";
        case TaskID::Video:
            return "Video";
        case TaskID::Managers:
            return "Managers";
        case TaskID::MQTT:
            return "MQTT";
        default:
            return "(invalid)";
    }
}

inline void PrintAllTaskIDs()
{
    std::cout << "List of TaskID:" << std::endl;
    std::cout << static_cast<int>(TaskID::Unknown) << ": " << TaskIDToString(TaskID::Unknown) << std::endl;
    std::cout << static_cast<int>(TaskID::Console) << ": " << TaskIDToString(TaskID::Console) << std::endl;
    std::cout << static_cast<int>(TaskID::Sender) << ": " << TaskIDToString(TaskID::Sender) << std::endl;
    std::cout << static_cast<int>(TaskID::Receiver) << ": " << TaskIDToString(TaskID::Receiver) << std::endl;
    std::cout << static_cast<int>(TaskID::Audio) << ": " << TaskIDToString(TaskID::Audio) << std::endl;
    std::cout << static_cast<int>(TaskID::Video) << ": " << TaskIDToString(TaskID::Video) << std::endl;
    std::cout << static_cast<int>(TaskID::Managers) << ": " << TaskIDToString(TaskID::Managers) << std::endl;
    std::cout << static_cast<int>(TaskID::MQTT) << ": " << TaskIDToString(TaskID::MQTT) << std::endl;
}

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
