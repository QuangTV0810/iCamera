// aiotek_mqtt_task.cpp
#include "aiotek_task_test.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include "mailbox.hpp"


void ThreadMQTTHandler(Task& task) {
    while (task.IsOperation()) {
        if (task.IsSuspended()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        // Try receive messages for this task
        auto packet = aiotek::core::m_mailbox.receive(3, std::chrono::milliseconds(500));
        if (packet) {
            // Process packet (e.g., MQTT publish based on signal)
            std::cout << "ThreadMQTTHandler received signal: " << packet->msg.signal << std::endl;
        }
        
        // std::cout << "ThreadMQTTHandler is running" << std::endl;
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void ThreadStreamHandler(Task& task) {
    while (task.IsOperation()) {
        if (task.IsSuspended()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Pause nhẹ
            continue;
        }

        // std::cout << "ThreadStreamHandler is running" << std::endl;
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}