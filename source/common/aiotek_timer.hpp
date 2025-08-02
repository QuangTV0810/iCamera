#ifndef __AIOTEK_TIMER_HPP__
#define __AIOTEK_TIMER_HPP__

#include <chrono>
#include <string>

namespace AIOTEK {

class Timer {
private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;
    bool state;

public:
    Timer();
    
    void start();
    void stop();
    void reset();
    
    double getElapsedSeconds() const;
    double getElapsedMilliseconds() const;
    double getElapsedMicroseconds() const;
    
    bool running() const;
    
    std::string getElapsedString() const;
};

} // namespace AIOTEK

#endif /* __AIOTEK_TIMER_HPP__ */
