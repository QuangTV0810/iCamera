#include "aiotek_task.hpp"
#include <pthread.h>
#include <iostream>

namespace aiotek {
namespace core {

std::vector<std::shared_ptr<Task>> TaskManager::m_tasks;
std::mutex TaskManager::m_manager_mutex;

Task::Task(const std::string& name, int id, std::function<void(Task&)> handler)
    : m_task_name(name), m_task_id(id), m_handler(handler), m_is_task_running(false), m_is_suspended(false), m_mailbox(32, id)
{
}

Task::~Task()
{
    Stop();
}

void Task::Start()
{
    if (m_is_task_running.exchange(true))
        return;
    std::lock_guard<std::mutex> lock(m_task_mutex);
    m_thread = std::thread(m_handler, std::ref(*this));
}

void Task::Stop()
{
    if (!m_is_task_running.exchange(false))
        return;
    {
        std::lock_guard<std::mutex> lock(m_task_mutex);
        m_is_suspended = false;
    }
    if (m_thread.joinable())
        m_thread.join();
}

void Task::Suspend()
{
    std::lock_guard<std::mutex> lock(m_task_mutex);
    m_is_suspended = true;
}

void Task::Resume()
{
    std::lock_guard<std::mutex> lock(m_task_mutex);
    m_is_suspended = false;
}

bool Task::IsOperation() const
{
    return m_is_task_running.load();
}

bool Task::IsSuspended() const
{
    std::lock_guard<std::mutex> lock(m_task_mutex);
    return m_is_suspended;
}

std::vector<TaskManager::TaskInfo>& TaskManager::GetTaskRegistry()
{
    static std::vector<TaskInfo> registry;
    return registry;
}

std::mutex& TaskManager::GetRegistryMutex()
{
    static std::mutex registry_mutex;
    return registry_mutex;
}

void TaskManager::RegisterTask(std::function<void(Task&)> handler, const std::string& name, int id)
{
    std::lock_guard<std::mutex> lock(GetRegistryMutex());
    GetTaskRegistry().push_back({handler, name, id});
    std::cout << "Register task: " << name << std::endl;
}

void TaskManager::StartAll()
{
    std::lock_guard<std::mutex> lock(m_manager_mutex);
    m_tasks.clear();
    for (const auto& task_info : GetTaskRegistry()) {
        auto task = std::make_shared<Task>(task_info.name, task_info.id, task_info.handler);
        m_tasks.push_back(task);
        task->Start();
        std::cout << "Task: " << task_info.name << " started" << std::endl;
    }
}

void TaskManager::StopAll()
{
    std::lock_guard<std::mutex> lock(m_manager_mutex);
    for (auto& task : m_tasks)
        task->Stop();
    m_tasks.clear();
}

void TaskManager::SuspendAll()
{
    std::lock_guard<std::mutex> lock(m_manager_mutex);
    for (auto& task : m_tasks)
        task->Suspend();
}

void TaskManager::ResumeAll()
{
    std::lock_guard<std::mutex> lock(m_manager_mutex);
    for (auto& task : m_tasks)
        task->Resume();
}

Mailbox* TaskManager::GetMailbox(int task_id)
{
    std::lock_guard<std::mutex> lock(m_manager_mutex);
    for (auto& task : m_tasks) {
        if (task->GetId() == task_id) {
            return &task->GetMailbox();
        }
    }
    return nullptr;
}

} // namespace core
} // namespace aiotek