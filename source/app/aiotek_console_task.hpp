#ifndef __AIOTEK_CONSOLE_TASK_HPP__
#define __AIOTEK_CONSOLE_TASK_HPP__

#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <functional>
#include "aiotek_task.hpp"
#include "aiotek_mailbox.hpp"
#include "aiotek_console.hpp"

namespace AIOTEK {

enum class ConsoleSignal : int32_t {
    // Thread control signals
    TERMINATE_THREAD = -1,
};
namespace app {
class ConsoleTask : public Task {
  public:
    ConsoleTask(int id);
    ~ConsoleTask();

    void init() override;
    void deinit() override;
    void start() override;
    void stop() override;

  private:
    bool isOperation();
    void setupConsoleCommands();
    void threadFunc();
    std::unique_ptr<aiotek::console::Console> m_console;
    std::atomic<bool> m_is_thread_need_stop;
    std::atomic<bool> m_is_thread_running;
};

} // namespace app
} // namespace AIOTEK

#endif // AIOTEK_CONSOLE_TASK_HPP