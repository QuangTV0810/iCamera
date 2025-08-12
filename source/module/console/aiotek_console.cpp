#include "aiotek_console.hpp"

namespace aiotek {
namespace console {

Console::Console() : m_is_console_thread_running(false), m_command_count(0)
{
}

Console::~Console()
{
    deinit();
}

void Console::init()
{
    std::cout << "======= AIOTEK Console Initializing... =======" << std::endl;
    initializeDefaultCommands();
    std::cout << "Default commands registered. Console ready." << std::endl;
}

void Console::deinit()
{
    stop();
    m_commands.clear();
    m_categories.clear();
    m_text_commands.clear();
    std::cout << "Console deinitialized." << std::endl;
}

void Console::start()
{
    if (m_is_console_thread_running.load()) {
        std::cout << "Console thread is already running." << std::endl;
        return;
    }
    m_is_console_thread_running.store(true);
    m_console_thread = std::thread(&Console::threadConsole, this);
    std::cout << "Console started. Type 'help' or 'menu' for options." << std::endl;
}

void Console::stop()
{
    if (!m_is_console_thread_running.load()) {
        return;
    }
    std::cout << "Stopping console thread..." << std::endl;
    m_is_console_thread_running.store(false);

    if (m_console_thread.joinable()) {
        m_console_thread.join();
    }
    std::cout << "Console thread stopped." << std::endl;
}

bool Console::isRunning() const
{
    return m_is_console_thread_running.load();
}

void Console::threadConsole()
{
    help();
    while (m_is_console_thread_running.load()) {
        
        getCmd();

        if (m_current_cmd.empty()) {
            continue;
        }

        if (isValidate()) {
            executeCommand();
        } else {
            std::cout << "Error: Unknown command '" << m_current_cmd << "'. Type 'help' for a list of commands." << std::endl;
        }
    }
}

void Console::getCmd()
{
    std::cout << "aiotek$ ";
    std::string line;
    if (std::getline(std::cin, line)) {
        parseCommand(line);
    } else {
        m_is_console_thread_running.store(false);
        m_current_cmd = "exit";
        m_cmd_args.clear();
    }
}

void Console::parseCommand(const std::string& input)
{
    std::stringstream ss(input);
    std::string token;

    m_cmd_args.clear();
    m_current_cmd.clear();

    if (ss >> token) {
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        m_current_cmd = token;
    }

    while (ss >> token) {
        m_cmd_args.push_back(token);
    }
}

void Console::registerCommand(const SettingCmd& cmd)
{
    std::lock_guard<std::mutex> lock(m_console_mutex);
    if (m_commands.count(cmd.id)) {
        std::cerr << "Warning: Overwriting command with ID " << cmd.id << std::endl;
    }
    m_commands[cmd.id] = cmd;
    m_categories[cmd.category].push_back(cmd.id);

    std::sort(m_categories[cmd.category].begin(), m_categories[cmd.category].end());
}

void Console::registerCommand(int id, const std::string& desc, const std::string& category,
                              std::function<void(const std::vector<std::string>&)> handler, bool needs_input, const std::string& prompt)
{
    SettingCmd cmd(id, desc, category, handler, needs_input, prompt);
    registerCommand(cmd);
}

void Console::unregisterCommand(int id)
{
    std::lock_guard<std::mutex> lock(m_console_mutex);

    auto it = m_commands.find(id);
    if (it != m_commands.end()) {
        std::string category = it->second.category;
        m_commands.erase(it);

        auto& cat_vec = m_categories[category];
        cat_vec.erase(std::remove(cat_vec.begin(), cat_vec.end(), id), cat_vec.end());

        if (cat_vec.empty()) {
            m_categories.erase(category);
        }
    }
}

bool Console::hasCommand(int id)
{
    std::lock_guard<std::mutex> lock(m_console_mutex);
    return m_commands.find(id) != m_commands.end();
}

std::vector<int> Console::getCommandsByCategory(const std::string& category)
{
    std::lock_guard<std::mutex> lock(m_console_mutex);
    auto it = m_categories.find(category);
    return (it != m_categories.end()) ? it->second : std::vector<int>();
}

void Console::clearCategory(const std::string& category)
{
    std::lock_guard<std::mutex> lock(m_console_mutex);

    auto it = m_categories.find(category);
    if (it != m_categories.end()) {
        for (int id : it->second) {
            m_commands.erase(id);
        }
        m_categories.erase(it);
    }
}

void Console::initializeDefaultCommands()
{
    // Text commands
    m_text_commands["help"] = [this]() { this->help(); };
    m_text_commands["h"] = [this]() { this->help(); };
    m_text_commands["menu"] = [this]() { this->help(); };
    m_text_commands["clear"] = [this]() { this->clearScreen(); };
    m_text_commands["cls"] = [this]() { this->clearScreen(); };

    auto exit_func = [this]() {
        std::cout << "Exiting console..." << std::endl;
        m_is_console_thread_running.store(false);
    };
    m_text_commands["exit"] = exit_func;
    m_text_commands["quit"] = exit_func;

    registerCommand(0, "Exit console", "System", [this](const auto& args) {
        (void) args;
        m_is_console_thread_running.store(false);
    });

    std::cout << "Default menu commands (using the new handler signature)." << std::endl;
}

void Console::help()
{
    std::lock_guard<std::mutex> lock(m_console_mutex);
    std::cout << "\n======= AIOTEK Console Menu =======" << std::endl;
    std::cout << "Choose an option by entering its number." << std::endl;
    std::cout << "-----------------------------------" << std::endl;

    for (const auto& [category, command_ids] : m_categories) {
        std::cout << "--- " << category << " Operations ---" << std::endl;

        for (int id : command_ids) {
            auto cmd_it = m_commands.find(id);
            if (cmd_it != m_commands.end()) {
                std::cout << "  " << std::left << std::setw(4) << id << "-> " << cmd_it->second.description << std::endl;
            }
        }
        std::cout << std::endl;
    }

    std::cout << "--- Text Commands ---" << std::endl;
    std::cout << "  help, h, menu   - Show this menu" << std::endl;
    std::cout << "  clear, cls      - Clear the screen" << std::endl;
    std::cout << "  exit, quit      - Exit the console" << std::endl;
    std::cout << "========================================" << std::endl;
}

void Console::clearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
    std::cout << "======= AIOTEK Console =======" << std::endl;
}

bool Console::isValidate()
{
    std::lock_guard<std::mutex> lock(m_console_mutex);

    try {
        std::size_t pos;
        int option = std::stoi(m_current_cmd, &pos);
        if (pos == m_current_cmd.length()) {
            return m_commands.count(option);
        }
    } catch (const std::exception&) {
    }

    return m_text_commands.count(m_current_cmd);
}

void Console::executeCommand()
{
    try {
        std::size_t pos;
        int option = std::stoi(m_current_cmd, &pos);
        if (pos == m_current_cmd.length()) {
            std::function<void(const std::vector<std::string>&)> callbackFunction;
            bool requires_input = false;
            std::string prompt;
            {
                std::lock_guard<std::mutex> lock(m_console_mutex);
                auto it = m_commands.find(option);
                if (it != m_commands.end()) {
                    callbackFunction = it->second.handler;
                    requires_input = it->second.requires_input;
                    prompt = it->second.input_prompt;
                }
            }

            if (callbackFunction) {
                if (requires_input && !prompt.empty()) {
                    std::cout << prompt;
                }
                callbackFunction(m_cmd_args);
                m_command_count++;
            }

            return;
        }
    } catch (const std::exception&) {
    }

    std::function<void()> callbackFunctionForTextCmd;
    {
        std::lock_guard<std::mutex> lock(m_console_mutex);
        auto it = m_text_commands.find(m_current_cmd);
        if (it != m_text_commands.end()) {
            callbackFunctionForTextCmd = it->second;
        }
    }

    if (callbackFunctionForTextCmd) {
        callbackFunctionForTextCmd();
        m_command_count++;
    }
}

} // namespace console
} // namespace aiotek