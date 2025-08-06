#ifndef AIOTEK_CONSOLE_HPP
#define AIOTEK_CONSOLE_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace aiotek {
namespace console {

struct SettingCmd {
    int id;
    std::string description;
    std::string category;
    std::function<void(const std::vector<std::string>&)> handler;
    bool requires_input = false;
    std::string input_prompt = "";

    SettingCmd() = default;
    SettingCmd(int _id, const std::string& _desc, const std::string& _cat,
                std::function<void(const std::vector<std::string>&)> _handler, 
                bool _input = false, const std::string& _prompt = "")
        : id(_id), description(_desc), category(_cat), handler(_handler),
          requires_input(_input), input_prompt(_prompt) {}
};

class Console {
public:
    Console();
    ~Console();

    void init();
    void deinit();
    void start();
    void stop();
    bool isRunning() const;

    void registerCommand(const SettingCmd& cmd);
    void registerCommand(int id, const std::string& desc, const std::string& category,
                         std::function<void(const std::vector<std::string>&)> handler, 
                         bool needs_input = false, const std::string& prompt = "");
    
    void unregisterCommand(int id);
    bool hasCommand(int id);
    std::vector<int> getCommandsByCategory(const std::string& category);
    void clearCategory(const std::string& category);

private:

    void threadConsole();
    void getCmd();
    void parseCommand(const std::string& input);
    bool isValidate();
    void executeCommand();
    void initializeDefaultCommands();
    void help();
    void clearScreen();

    std::thread m_console_thread;
    std::atomic<bool> m_is_console_thread_running;
    std::mutex m_console_mutex;
    std::map<int, SettingCmd> m_commands;
    std::map<std::string, std::vector<int>> m_categories;
    std::map<std::string, std::function<void()>> m_text_commands;
    std::string m_current_cmd;
    std::vector<std::string> m_cmd_args;
    size_t m_command_count = 0;
};

} // namespace console
} // namespace aiotek

#endif // AIOTEK_CONSOLE_HPP