#include "aiotek_task_list.hpp"
#include "task.hpp"
#include "aiotek_task_test.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include "mailbox.hpp"

extern void ThreadMQTTHandler(Task& task);
extern void ThreadStreamHandler(Task& task);

// Register tasks with TaskManager
void TaskList()
{
    // TaskManager::RegisterTask(
    //     [](Task& task) {
    //         while (task.IsOperation()) {
    //             if (task.IsSuspended()) {
    //                 std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //                 continue;
    //             }
    //             // Try receive message for this task
    //             auto packet = aiotek::core::m_mailbox.try_receive(task.IsOperation()); // Use task ID
    //             if (packet) {
    //                 // Process packet (e.g., log signal)
    //                 std::cout << "Video received signal: " << packet->msg.signal << std::endl;
    //             }
    //             // Video capture logic
    //         }
    //     },
    //     "Video_Task", 3);

    // TaskManager::RegisterTask(
    //     [](Task& task) {
    //         while (task.IsOperation()) {
    //             if (task.IsSuspended()) {
    //                 std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //                 continue;
    //             }
    //             // MQTT task sends a message to Video_Task
    //             aiotek::core::MailboxPacket packet{task.IsOperation(), 1, {100, "MQTT message", 0}};
    //             aiotek::core::m_mailbox.send(packet);

    //             // aiotek::core::MailboxPacket packet2 = {task.IsOperation(), 3, {300, "Video message", 0}};
    //             // aiotek::core::m_mailbox.send(packet);
    //         }
    //     },
    //     "MQTT_Task", 4);

    TaskManager::RegisterTask(
        [](Task& task) {
            while (task.IsOperation()) {
                if (task.IsSuspended()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }

                aiotek::core::MailboxPacket packet{1, 3, {12345, "MQTT message", 0}};
                aiotek::core::m_mailbox.send(packet);

                aiotek::core::MailboxPacket packet2 = {1, 2, {300, "Video message", 0}};
                aiotek::core::m_mailbox.send(packet2);

                // std::cout << "Test_1_Task is running" << std::endl;
                // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        },
        "Test_1_Task", 1);

    TaskManager::RegisterTask(
        [](Task& task) {
            while (task.IsOperation()) {
                if (task.IsSuspended()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                // Try receive message for this task
                auto packet = aiotek::core::m_mailbox.try_receive(2); // Use task ID
                if (packet) {
                    std::cout << "Test_2_Task received signal: " << packet->msg.signal << std::endl;
                }
                // std::cout << "Test_2_Task is running" << std::endl;
                // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        },
        "Test_2_Task", 2);

    TaskManager::RegisterTask(ThreadMQTTHandler, "MQTT_Task", 3);
    TaskManager::RegisterTask(ThreadStreamHandler, "Stream_Task", 4);
}