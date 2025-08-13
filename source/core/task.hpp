#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <functional>
#include <vector>
#include <memory>

/**
 * @brief Task class to manage a single thread with a custom handler.
 *        Designed for camera project on Luckfox Pico, supports start/stop/suspend/resume.
 */
class Task {
public:
    /**
     * @brief Construct a Task with name, ID, and handler.
     * @param name Task name for identification/debugging.
     * @param id Unique task ID.
     * @param handler Function to run in thread, takes Task& to check state.
     */
    Task(const std::string& name, int id, std::function<void(Task&)> handler);

    /**
     * @brief Destructor, auto stops and joins thread.
     */
    virtual ~Task();

    /**
     * @brief Start the task (spawn thread with handler).
     */
    void Start();

    /**
     * @brief Stop the task (set flag and join thread).
     */
    void Stop();

    /**
     * @brief Suspend the task (set flag, handler should check).
     */
    void Suspend();

    /**
     * @brief Resume the task (clear flag, handler continues).
     */
    void Resume();

    /**
     * @brief Check if task is running.
     * @return true if thread is active.
     */
    bool IsOperation() const;

    /**
     * @brief Check if task is suspended (for handler to pause).
     * @return true if suspended.
     */
    bool IsSuspended() const;

private:
    std::string m_task_name;                    // Task name for debug
    int m_task_id;                              // Unique task ID
    std::function<void(Task&)> m_handler;       // Handler to run in thread
    std::atomic<bool> m_is_task_running;        // Thread-safe running flag
    bool m_is_suspended;                        // Suspend flag (protected by mutex)
    std::thread m_thread;                       // Thread object
    mutable std::mutex m_task_mutex;            // Mutex for sync, mutable for const methods
};

/**
 * @brief TaskManager class to manage multiple tasks.
 *        Handles registering and controlling tasks (start/stop/suspend/resume) concurrently.
 */
class TaskManager {
public:
    /**
     * @brief Register a task handler statically (called in task_list.cpp).
     * @param handler Function to run in task thread, takes Task& for state.
     * @param name Task name for identification.
     * @param id Unique task ID.
     */
    static void RegisterTask(std::function<void(Task&)> handler, const std::string& name, int id);

    /**
     * @brief Start all registered tasks concurrently.
     */
    void StartAll();

    /**
     * @brief Stop all tasks and join their threads.
     */
    void StopAll();

    /**
     * @brief Suspend all tasks (handlers should check suspend state).
     */
    void SuspendAll();

    /**
     * @brief Resume all tasks.
     */
    void ResumeAll();

private:
    std::vector<std::shared_ptr<Task>> m_tasks; // List of managed tasks
    std::mutex m_manager_mutex;                 // Mutex for task list

    // Static registry for tasks (populated in task_list.cpp)
    struct TaskInfo {
        std::function<void(Task&)> handler;
        std::string name;
        int id;
    };
    static std::vector<TaskInfo>& GetTaskRegistry();
    static std::mutex& GetRegistryMutex();      // Mutex for registry
};