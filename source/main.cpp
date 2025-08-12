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
    AIOTEK::managers.stop();
}

void setupConsoleCommands(aiotek::console::Console& console)
{
    // MQTT commands
    console.registerCommand(1, "Connect to MQTT broker", "MQTT", [](const auto& args) {
        (void) args;
        std::cout << "Executing: Connect to MQTT..." << std::endl;
        // AIOTEK::g_mailbox.send({...});
    });

    console.registerCommand(
        2, "Push data to MQTT", "MQTT",
        [](const auto& args) {
            (void) args;
            std::cout << "Executing: Push data..." << std::endl;
            std::string data;
            std::getline(std::cin, data);
            if (!data.empty()) {
                std::cout << "-> Pushing data: '" << data << "'" << std::endl;
            } else {
                std::cout << "-> No data entered." << std::endl;
            }
        },
        true, "Enter data to push: ");

    console.registerCommand(3, "Disconnect from MQTT", "MQTT", [](const auto& args) {
        (void) args;
        std::cout << "Executing: Disconnect from MQTT..." << std::endl;
    });

    console.registerCommand(4, "Show MQTT status", "MQTT", [](const auto& args) {
        (void) args;
        std::cout << "Status: MQTT is currently connected." << std::endl;
    });

    console.registerCommand(5, "Show system status", "System", [](const auto& args) {
        (void) args;
        std::cout << "Status: System is running normally." << std::endl;
    });
}

int main()
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::cout << "iCamera starting..." << std::endl;

    AIOTEK::managers.init();
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
