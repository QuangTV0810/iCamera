#ifndef AIOTEK_TASK_HPP
#define AIOTEK_TASK_HPP

#include <vector>
#include <thread>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include "aiotek_log.hpp"
#include "aiotek_timer.hpp"

namespace AIOTEK {

class Task {
public:
    Task(const std::string& name, int id)
        : m_name(name), m_id(id), m_running(false) {}
    virtual ~Task() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool state() const { return m_running; }
    std::string name() const { return m_name; }
    int id() const { return m_id; }

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