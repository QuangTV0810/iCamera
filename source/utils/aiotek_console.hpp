#pragma once
#include <string>

extern volatile bool g_shutdown_requested;
std::string aiotek_console_readline(const std::string& prompt = "> ");