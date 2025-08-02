#include "aiotek_console.hpp"
#include <iostream>

volatile bool g_shutdown_requested = false;
std::string aiotek_console_readline(const std::string& prompt) {
    std::string line;
    std::cout << prompt;
    std::getline(std::cin, line);
    return line;
}
