#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include "aiotek_mailbox.hpp"

namespace aiotek {
namespace core {

class Task {
  public:
    Task(const std::string& name, int id, std::function<void(Task&)> handler);
    virtual ~Task();
    void Start();
    void Stop();
    void Suspend();
    void Resume();
    bool IsOperation() const;
    bool IsSuspended() const;
    int GetId() const
    {
        return m_task_id;
    }
    Mailbox& GetMailbox()
    {
        return m_mailbox;
    }

  private:
    std::string m_task_name;
    int m_task_id;
    std::function<void(Task&)> m_handler;
    std::atomic<bool> m_is_task_running;
    bool m_is_suspended;
    std::thread m_thread;
    mutable std::mutex m_task_mutex;
    Mailbox m_mailbox;
};

class TaskManager {
  public:
    static void RegisterTask(std::function<void(Task&)> handler, const std::string& name, int id);
    static void StartAll();
    static void StopAll();
    static void SuspendAll();
    static void ResumeAll();
    static Mailbox* GetMailbox(int task_id);

  private:
    static std::vector<std::shared_ptr<Task>> m_tasks;
    static std::mutex m_manager_mutex;

    struct TaskInfo {
        std::function<void(Task&)> handler;
        std::string name;
        int id;
    };
    static std::vector<TaskInfo>& GetTaskRegistry();
    static std::mutex& GetRegistryMutex();
};

} // namespace core
} // namespace aiotek