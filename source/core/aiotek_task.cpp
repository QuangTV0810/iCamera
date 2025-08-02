#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <functional>
#include <string>
#include "aiotek_log.hpp"
#include "aiotek_timer.hpp"
#include "aiotek_net_managers.hpp"
#include "aiotek_task.hpp"
#include "aiotek_mqtt_task.hpp"


using namespace AIOTEK;
extern void task_sender();
extern void task_receiver();
extern void task_mqtt();

namespace AIOTEK {

TaskManagers managers;

TaskManagers::TaskManagers() : m_running(false) {
    addTask(std::make_unique<MQTTTask>(1));
}

TaskManagers::~TaskManagers() {
    stop();
}

bool TaskManagers::start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_running)
        return true;
    AIOTEK_LOG_INFO("TaskManagers: Starting");
    m_running = true;

    for (auto& task : tasks_) {
        task->start();
        AIOTEK_LOG_INFO("TaskManagers: Started task " + task->name());
    }
    taskThread_ = std::thread(&TaskManagers::run, this);
    return true;
}

void TaskManagers::stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running)
        return;
    AIOTEK_LOG_INFO("TaskManagers: Stopping");
    m_running = false;
    for (auto& task : tasks_) {
        task->stop();
        AIOTEK_LOG_INFO("TaskManagers: Stopped task " + task->name());
    }
    if (taskThread_.joinable()) {
        taskThread_.join();
    }
}

bool TaskManagers::state() const {
    return m_running;
}

void TaskManagers::addTask(std::unique_ptr<Task> task) {
    std::lock_guard<std::mutex> lock(m_mutex);
    tasks_.push_back(std::move(task));
}

Task* TaskManagers::getTaskByName(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& task : tasks_) {
        if (task->name() == name) return task.get();
    }
    return nullptr;
}

Task* TaskManagers::getTaskById(int id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& task : tasks_) {
        if (task->id() == id) return task.get();
    }
    return nullptr;
}

std::vector<Task*> TaskManagers::getAllTasks() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Task*> result;
    for (auto& task : tasks_) {
        result.push_back(task.get());
    }
    return result;
}

void TaskManagers::run() {
    AIOTEK_LOG_INFO("TaskManagers: Thread started");
    timer_.start();
    while (m_running) {
        processManagers();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    timer_.stop();
    AIOTEK_LOG_INFO("TaskManagers: Thread stopped after " + timer_.getElapsedString());
}

void TaskManagers::processManagers() {
    static int counter = 0;
    if (++counter % 100 == 0) {
        AIOTEK_LOG_DEBUG("TaskManagers: Processing managers");
    }
}

} // namespace AIOTEK
