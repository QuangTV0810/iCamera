#ifndef __AIOTEK_CONSOLE_TASK_HPP__
#define __AIOTEK_CONSOLE_TASK_HPP__

#include <string>
#include "aiotek_task.hpp"
#include "aiotek_console.hpp"

namespace aiotek {
namespace app {

class ConsoleTask {
  public:
    ConsoleTask();
    ~ConsoleTask();

    void Initialize();
    void Deinitialize();
    void Start();
    void Stop();

    void ThreadConsoleHandler(aiotek::core::Task& task);

  private:
    void RegisterConsoleCommands();

  private:
    std::atomic<bool> m_is_thread_running;
    std::shared_ptr<aiotek::console::Console> m_console;
};

} // namespace app
} // namespace aiotek
#endif /* __AIOTEK_CONSOLE_TASK_HPP__ */