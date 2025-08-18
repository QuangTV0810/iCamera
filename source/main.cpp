#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>

#include "aiotek_logger.hpp"
#include "aiotek_timer.hpp"
#include "aiotek_net_if.hpp"
#include "aiotek_mqtt.hpp"
#include "aiotek_console.hpp"
#include "aiotek_task_list.hpp"
#include "aiotek_task.hpp"

volatile bool g_running = true;
aiotek::core::TaskManager task_manager;

void signal_handler(int signal)
{
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
    task_manager.StopAll();
}

int main()
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::cout << "iCamera starting..." << std::endl;

    try {
        AIOTEK_LOG_INFO("iCamera application started");

        aiotek::core::TimerManager::GetInstance().Start();

        aiotek::core::Timer timer1;
        int user_data1 = 42;
        timer1.Initialize(
            "Timer1", 100, aiotek::core::Timer::TimerType::ONESHOT,
            std::chrono::milliseconds(5000),
            [](aiotek::core::Timer* timer, void* data) {
                int value = *(int*)data;
                AIOTEK_LOG_INFO("Timer1 callback triggered, id={}, data={}", timer->GetId(), value);
                // Mailbox* mailbox = aiotek::core::TaskManager::GetMailbox(2);
                // if (mailbox) {
                //     MailboxPacket packet;
                //     packet.sender = timer->GetId();
                //     packet.receiver = 2;
                //     packet.msg.signal = timer->GetId();
                //     packet.msg.msg = "Timer1 triggered with value: " + std::to_string(value);
                //     packet.msg.len = packet.msg.msg.size();
                //     mailbox->send(packet);
                // }
            },
            &user_data1
        );
        timer1.Start(std::chrono::milliseconds(5000));

        aiotek::core::Timer timer2;
        int user_data2 = 99;
        timer2.Initialize(
            "Timer2", 101, aiotek::core::Timer::TimerType::PERIODIC,
            std::chrono::milliseconds(1000),
            [](aiotek::core::Timer* timer, void* data) {
                int value = *(int*)data;
                AIOTEK_LOG_INFO("Timer2 callback triggered, id={}, data={}", timer->GetId(), value);
                // Mailbox* mailbox = aiotek::core::TaskManager::GetMailbox(2);
                // if (mailbox) {
                //     MailboxPacket packet;
                //     packet.sender = timer->GetId();
                //     packet.receiver = 2;
                //     packet.msg.signal = timer->GetId();
                //     packet.msg.msg = "Timer2 triggered with value: " + std::to_string(value);
                //     packet.msg.len = packet.msg.msg.size();
                //     mailbox->send(packet);
                // }
            },
            &user_data2
        );
        timer2.Start(std::chrono::milliseconds(2000));

        aiotek::core::Timer timer3;
        int user_data3 = 123;
        timer3.Initialize(
            "Timer3", 102, aiotek::core::Timer::TimerType::PERIODIC,
            std::chrono::milliseconds(3000),
            [](aiotek::core::Timer* timer, void* data) {
                int value = *(int*)data;
                AIOTEK_LOG_INFO("Timer3 callback triggered, id={}, data={}", timer->GetId(), value);
                // Mailbox* mailbox = aiotek::core::TaskManager::GetMailbox(2);
                // if (mailbox) {
                //     MailboxPacket packet;
                //     packet.sender = timer->GetId();
                //     packet.receiver = 2;
                //     packet.msg.signal = timer->GetId();
                //     packet.msg.msg = "Timer3 triggered with value: " + std::to_string(value);
                //     packet.msg.len = packet.msg.msg.size();
                //     mailbox->send(packet);
                // }

                // if (value == 123) {
                //     timer->SetPeriod(std::chrono::milliseconds(5000));
                //     *(int*)data = 456;
                // } else if (value == 456) {
                //     timer->Delete();
                // }
            },
            &user_data3
        );
        timer3.Start(std::chrono::milliseconds(3000));

        // std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        // timer2.Pause();
        // AIOTEK_LOG_INFO("Main: Paused Timer2");
        // std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        // timer2.Resume();
        // AIOTEK_LOG_INFO("Main: Resumed Timer2");

        aiotek::app::RegisterAllTask();
        task_manager.StartAll();

        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        AIOTEK_LOG_INFO("iCamera application shutting down");

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        AIOTEK_LOG_ERROR("Application error: " + std::string(e.what()));
        return 1;
    }

    std::cout << "iCamera stopped" << std::endl;
    return 0;
}
