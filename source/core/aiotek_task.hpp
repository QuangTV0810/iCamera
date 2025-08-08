#ifndef AIOTEK_TASK_HPP
#define AIOTEK_TASK_HPP

#include <vector>
#include <thread>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <iostream>
#include "aiotek_log.hpp"
#include "aiotek_timer.hpp"

namespace AIOTEK {

enum class TaskID { Unknown = 0, Sender, Receiver, Audio, Video, Managers, MQTT_TASK_ID, PUSH_STREAM_TASK_ID, CONSOLE_TASK_ID };

inline const char* TaskIDToString(TaskID id)
{
    switch (id) {
        case TaskID::Unknown:
            return "Unknown";
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
        case TaskID::MQTT_TASK_ID:
            return "MQTT_TASK";
        case TaskID::PUSH_STREAM_TASK_ID:
            return "PUSH_STREAM_TASK";
        case TaskID::CONSOLE_TASK_ID:
            return "CONSOLE_TASK_ID";
        default:
            return "(invalid)";
    }
}

inline void PrintAllTaskIDs()
{
    std::cout << "List of TaskID:" << std::endl;
    std::cout << static_cast<int>(TaskID::Unknown) << ": " << TaskIDToString(TaskID::Unknown) << std::endl;
    std::cout << static_cast<int>(TaskID::Sender) << ": " << TaskIDToString(TaskID::Sender) << std::endl;
    std::cout << static_cast<int>(TaskID::Receiver) << ": " << TaskIDToString(TaskID::Receiver) << std::endl;
    std::cout << static_cast<int>(TaskID::Audio) << ": " << TaskIDToString(TaskID::Audio) << std::endl;
    std::cout << static_cast<int>(TaskID::Video) << ": " << TaskIDToString(TaskID::Video) << std::endl;
    std::cout << static_cast<int>(TaskID::Managers) << ": " << TaskIDToString(TaskID::Managers) << std::endl;
    std::cout << static_cast<int>(TaskID::MQTT_TASK_ID) << ": " << TaskIDToString(TaskID::MQTT_TASK_ID) << std::endl;
    std::cout << static_cast<int>(TaskID::PUSH_STREAM_TASK_ID) << ": " << TaskIDToString(TaskID::PUSH_STREAM_TASK_ID) << std::endl;
    std::cout << static_cast<int>(TaskID::CONSOLE_TASK_ID) << ": " << TaskIDToString(TaskID::CONSOLE_TASK_ID) << std::endl;
}

class Task {
  public:
    Task(const std::string& name, int id) : m_name(name), m_id(id), m_running(false)
    {
    }
    virtual ~Task() = default;
    virtual void init() = 0;
    virtual void deinit() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool state() const
    {
        return m_running;
    }
    std::string name() const
    {
        return m_name;
    }
    int id() const
    {
        return m_id;
    }

  protected:
    std::string m_name;
    int m_id;
    std::atomic<bool> m_running;
    std::thread m_thread;
    mutable std::mutex m_mutex;
};

class TaskManagers {
  public:
    TaskManagers();
    ~TaskManagers();
    bool init();
    bool start();
    void stop();
    bool state() const;
    void addTask(std::unique_ptr<Task> task);
    Task* getTaskByName(const std::string& name);
    Task* getTaskById(int id);
    std::vector<Task*> getAllTasks();

  private:
    bool m_running;
    std::thread taskThread_;
    Timer timer_;
    std::vector<std::unique_ptr<Task>> tasks_;
    mutable std::mutex m_mutex;
    void run();
    void processManagers();
};

extern TaskManagers managers;

} // namespace AIOTEK

#endif // AIOTEK_TASK_HPP