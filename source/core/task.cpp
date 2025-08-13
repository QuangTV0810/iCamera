#include "task.hpp"
#include <pthread.h>  // For pthread_setname_np (debug)

// Task implementation
Task::Task(const std::string& name, int id, std::function<void(Task&)> handler)
    : m_task_name(name),
      m_task_id(id),
      m_handler(handler),
      m_is_task_running(false),
      m_is_suspended(false) {
    // RAII: Initialize members, no thread spawned yet
}

Task::~Task() {
    Stop();  // Auto stop and join on destroy
}

void Task::Start() {
    if (m_is_task_running.exchange(true)) return;  // Prevent duplicate start
    std::lock_guard<std::mutex> lock(m_task_mutex);
    m_thread = std::thread(m_handler, std::ref(*this));  // Run handler directly
    // Set thread name for debugging on Linux
    pthread_setname_np(m_thread.native_handle(), m_task_name.substr(0, 15).c_str());
}

void Task::Stop() {
    if (!m_is_task_running.exchange(false)) return;  // Not running
    {
        std::lock_guard<std::mutex> lock(m_task_mutex);
        m_is_suspended = false;  // Clear suspend to allow handler to exit
    }
    if (m_thread.joinable()) m_thread.join();  // Wait for thread to finish
}

void Task::Suspend() {
    std::lock_guard<std::mutex> lock(m_task_mutex);
    m_is_suspended = true;  // Handler should check this
}

void Task::Resume() {
    std::lock_guard<std::mutex> lock(m_task_mutex);
    m_is_suspended = false;  // Handler can continue
}

bool Task::IsOperation() const {
    return m_is_task_running.load();  // Thread-safe check
}

bool Task::IsSuspended() const {
    std::lock_guard<std::mutex> lock(m_task_mutex);  // Lock mutable mutex
    return m_is_suspended;  // Protected by mutex
}

// TaskManager implementation
std::vector<TaskManager::TaskInfo>& TaskManager::GetTaskRegistry() {
    static std::vector<TaskInfo> registry;
    return registry;
}

std::mutex& TaskManager::GetRegistryMutex() {
    static std::mutex registry_mutex;
    return registry_mutex;
}

void TaskManager::RegisterTask(std::function<void(Task&)> handler, const std::string& name, int id) {
    std::lock_guard<std::mutex> lock(GetRegistryMutex());  // Use static mutex
    GetTaskRegistry().push_back({handler, name, id});
}

void TaskManager::StartAll() {
    std::lock_guard<std::mutex> lock(m_manager_mutex);
    // Create tasks from registry
    m_tasks.clear();  // Clear old tasks
    for (const auto& task_info : GetTaskRegistry()) {
        auto task = std::make_shared<Task>(task_info.name, task_info.id, task_info.handler);
        m_tasks.push_back(task);
        task->Start();
    }
}

void TaskManager::StopAll() {
    std::lock_guard<std::mutex> lock(m_manager_mutex);
    for (auto& task : m_tasks) task->Stop();
    m_tasks.clear();  // Clear tasks after stopping
}

void TaskManager::SuspendAll() {
    std::lock_guard<std::mutex> lock(m_manager_mutex);
    for (auto& task : m_tasks) task->Suspend();
}

void TaskManager::ResumeAll() {
    std::lock_guard<std::mutex> lock(m_manager_mutex);
    for (auto& task : m_tasks) task->Resume();
}