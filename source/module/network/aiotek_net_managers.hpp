#ifndef __AIOTEK_NET_MANAGERS_HPP__
#define __AIOTEK_NET_MANAGERS_HPP__

#include <vector>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "aiotek_net_if.hpp"

namespace AIOTEK_NetworkManagers {
extern std::unordered_map<std::string, std::unique_ptr<AIOTEK_NetworkInterface>> managers;
extern std::mutex managers_mutex;
class NetworkManagers {
  private:
  public:
    NetworkManagers();
    ~NetworkManagers();
};
} // namespace AIOTEK_NetworkManagers

#endif /* __AIOTEK_NET_MANAGERS_HPP__ */