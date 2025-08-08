#include <chrono>
#include "aiotek_console_task.hpp"

namespace AIOTEK {
namespace app {
ConsoleTask::ConsoleTask(int id) : Task("Console", id), m_is_thread_need_stop(false), m_is_thread_running(false)
{
    m_console = std::make_unique<aiotek::console::Console>();
}
ConsoleTask::~ConsoleTask()
{
    stop();
    deinit();
}

void ConsoleTask::init()
{
    if (m_console) {
        m_console->init();
    }
}

void ConsoleTask::deinit()
{
    if (m_console) {
        m_console->deinit();
    }
}

void ConsoleTask::start()
{
    if (m_is_thread_running.load()) {
        return;
    }

    if (m_console) {
        setupConsoleCommands();
        m_console->start();
    }

    m_is_thread_need_stop.store(false);
    m_is_thread_running.store(true);
    m_running.store(true);

    m_thread = std::thread([this]() -> void {
        this->threadFunc();
        this->m_is_thread_running.store(false);
        this->m_running.store(false);
    });
}

void ConsoleTask::stop()
{
    if (!m_is_thread_running.load() && !m_thread.joinable()) {
        return;
    }

    if (m_console) {
        m_console->stop();
    }

    m_is_thread_need_stop.store(true);
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

bool ConsoleTask::isOperation()
{
    return m_is_thread_running;
}

void ConsoleTask::setupConsoleCommands()
{
    if (!m_console)
        return;

    m_console->registerCommand(1, "Connect to MQTT broker", "MQTT", [](const auto& args) {
        (void) args;
        std::cout << "Executing: Connect to MQTT..." << std::endl;
        // AIOTEK::g_mailbox.send({...});
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
        std::cout << "\n=== Active Tasks ===" << std::endl;
        auto tasks = AIOTEK::managers.getAllTasks();
        int activeCount = 0;
        for (auto* task : tasks) {
            if (task && task->state()) {
                std::cout << "- [" << task->id() << "] " << task->name() << std::endl;
                activeCount++;
            }
        }
        if (activeCount == 0) {
            std::cout << "(none)" << std::endl;
        }
    });
}

void ConsoleTask::threadFunc()
{
    while (!m_is_thread_need_stop.load()) {
        AIOTEK::MailboxPacket packet = AIOTEK::g_mailbox.receive();
        if (packet.receiver == AIOTEK::TaskID::MQTT_TASK_ID) {
            std::cout << "[ConsoleTask] From: " << AIOTEK::TaskIDToString(packet.sender) << " To: " << AIOTEK::TaskIDToString(packet.receiver)
                      << std::endl;
            switch (packet.msg.signal) {
                case static_cast<int32_t>(ConsoleSignal::TERMINATE_THREAD):
                    AIOTEK_LOG_INFO("ConsoleTask: Received terminate signal, stopping thread");
                    m_is_thread_running = false;
                    break;
                default:
                    AIOTEK_LOG_DEBUG("ConsoleTask: Unknown signal: " + std::to_string(packet.msg.signal));
                    break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

} // namespace app
} // namespace AIOTEK