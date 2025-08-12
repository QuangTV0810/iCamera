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
#include "aiotek_push_stream_task.hpp"
#include "aiotek_console_task.hpp"
#include "aiotek_rtmp_push_stream_task.hpp"

namespace AIOTEK {

TaskManagers managers;

TaskManagers::TaskManagers() {
    // addTask(std::make_unique<MQTTTask>(static_cast<int>(AIOTEK::TaskID::MQTT_TASK_ID)));
    // addTask(std::make_unique<PushStreamTask>(static_cast<int>(AIOTEK::TaskID::PUSH_STREAM_TASK_ID)));
    addTask(std::make_unique<app::RTMPPushTask>(static_cast<int>(AIOTEK::TaskID::PUSH_RTMP_TASK_ID)));
    // addTask(std::make_unique<app::ConsoleTask>(static_cast<int>(AIOTEK::TaskID::CONSOLE_TASK_ID)));
}

TaskManagers::~TaskManagers() {
    stop();
}

bool TaskManagers::init() {
    // std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& task : tasks_) {
        AIOTEK_LOG_INFO("TaskManagers: Initialize task " + task->name());
        task->init();
    }

    return true;
}

bool TaskManagers::start() {
    // std::lock_guard<std::mutex> lock(m_mutex);
    // if (m_running)
    //     return true;

    // m_running = true;

    for (auto& task : tasks_) {
        task->start();
        AIOTEK_LOG_INFO("TaskManagers: Started task " + task->name());
    }

    return true;
}

void TaskManagers::stop() {
    // std::lock_guard<std::mutex> lock(m_mutex);
    // if (!m_running)
    //     return;

    // m_running = false;
    for (auto& task : tasks_) {
        AIOTEK_LOG_INFO("TaskManagers: Stopping task " + task->name());
        task->stop();
    }
}

bool TaskManagers::state() const {
    return true;
}

void TaskManagers::addTask(std::unique_ptr<Task> task) {
    std::lock_guard<std::mutex> lock(m_task_manager_mutex);
    tasks_.push_back(std::move(task));
}

Task* TaskManagers::getTaskByName(const std::string& name) {
    // std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& task : tasks_) {
        if (task->name() == name) return task.get();
    }
    return nullptr;
}

Task* TaskManagers::getTaskById(int id) {
    // std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& task : tasks_) {
        if (task->id() == id) return task.get();
    }
    return nullptr;
}

std::vector<Task*> TaskManagers::getAllTasks() {
    // std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Task*> result;
    for (auto& task : tasks_) {
        result.push_back(task.get());
    }
    return result;
}


} // namespace AIOTEK
