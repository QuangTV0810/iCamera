#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>

#include "aiotek_log.hpp"
#include "aiotek_timer.hpp"
#include "aiotek_net_if.hpp"
#include "aiotek_mqtt.hpp"
#include "aiotek_task.hpp"
#include "aiotek_console.hpp"

volatile bool g_running = true;

void signal_handler(int signal)
{
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
    g_shutdown_requested = true;
}

int main()
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::cout << "iCamera starting..." << std::endl;

    AIOTEK::managers.start();
    try {
        AIOTEK_LOG_INFO("iCamera application started");

        AIOTEK::Timer timer;
        timer.start();

        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        AIOTEK_LOG_INFO("iCamera application shutting down");

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        AIOTEK_LOG_ERROR("Application error: " + std::string(e.what()));
        AIOTEK::managers.stop();
        return 1;
    }
    AIOTEK::managers.stop();
    std::cout << "iCamera stopped" << std::endl;
    return 0;
}
