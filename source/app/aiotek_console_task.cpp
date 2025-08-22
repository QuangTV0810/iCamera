#include <chrono>
#include "aiotek_console_task.hpp"
#include "aiotek_task_list.hpp"

namespace aiotek {
namespace app {
ConsoleTask::ConsoleTask() : m_is_thread_running(false)
{
    m_console = std::make_shared<aiotek::console::Console>();
}
ConsoleTask::~ConsoleTask()
{
    m_console->deinit();
}

void ConsoleTask::Initialize()
{
    if (m_console) {
        m_console->init();
    }
}

void ConsoleTask::Deinitialize()
{
    if (m_console) {
        m_console->deinit();
    }
}

void ConsoleTask::Start()
{
    m_console->start();
}

void ConsoleTask::Stop()
{
}

void ConsoleTask::RegisterConsoleCommands()
{
    if (!m_console)
        return;

    m_console->registerCommand(1, "Connect to MQTT broker", "MQTT", [](const auto& args) {
        (void) args;
        std::cout << "Executing: Connect to MQTT..." << std::endl;
    });

    m_console->registerCommand(
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

    m_console->registerCommand(3, "Disconnect from MQTT", "MQTT", [](const auto& args) {
        (void) args;
        std::cout << "Executing: Disconnect from MQTT..." << std::endl;
    });

    m_console->registerCommand(4, "Show MQTT status", "MQTT", [](const auto& args) {
        (void) args;
        std::cout << "Status: MQTT is currently connected." << std::endl;
    });

    m_console->registerCommand(5, "Show system status", "System", [](const auto& args) {
        (void) args;
        std::cout << "Status: System is running normally." << std::endl;
    });

    m_console->registerCommand(6, "List active tasks", "System", [](const auto& args) {
        (void) args;
        auto active_tasks = core::TaskManager::GetActiveTasks();
    });
}

void ConsoleTask::ThreadConsoleHandler(aiotek::core::Task& task)
{
    this->RegisterConsoleCommands();
    this->Initialize();
    this->Start();

    try {
        while (task.IsOperation()) {
            if (task.IsSuspended()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            auto packet = task.GetMailbox().receive(std::chrono::milliseconds(50));
            if (packet) {
                if (CONSOLE_TASK == packet->task_sender_id) {
                } else {
                }
            }
        }
    } catch (const std::exception& e) {
        std::cout << "ConsoleTask: Exception in handler: " << e.what() << std::endl;
    }
}

} // namespace app
} // namespace aiotek