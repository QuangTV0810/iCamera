#pragma once

#include "aiotek_task.hpp"
#include <chrono>
#include <functional>
#include <string>
#include <list>
#include <mutex>
#include <condition_variable>

namespace aiotek {
namespace core {

class Timer {
  public:
    enum class TimerType { ONESHOT, PERIODIC };

    using TimerCallback = std::function<void(Timer*, void*)>;

    Timer() = default;
    ~Timer();

    void Initialize(const std::string& name, int id, TimerType type, std::chrono::milliseconds period, TimerCallback callback, void* user_data);

    void Deinitialize();

    void Start(std::chrono::milliseconds start_time = std::chrono::milliseconds(0));

    void Stop();

    void Pause();

    void Resume();

    void SetPeriod(std::chrono::milliseconds new_period);

    void Delete();

    bool IsActivated() const;

    const std::string& GetName() const
    {
        return m_name;
    }
    int GetId() const
    {
        return m_id;
    }

  private:
    friend class TimerManager;

    std::chrono::steady_clock::time_point m_deadline;
    std::chrono::milliseconds m_period;
    std::chrono::milliseconds m_remaining_time{0};
    TimerType m_type;
    TimerCallback m_callback;
    void* m_user_data;
    bool m_activated{false};
    std::string m_name;
    int m_id;
};

class TimerManager {
  public:
    static TimerManager& GetInstance();

    void Start();

    void Stop();

    void AddTimer(Timer* timer);

    void RemoveTimer(Timer* timer);

    void UpdateDeadline(Timer* timer, std::chrono::milliseconds new_deadline);

    bool IsTimerInList(const Timer* timer) const;

    void ThreadTimerHandler(Task& task);

  private:
    TimerManager();
    ~TimerManager();

    std::list<Timer*> m_timer_list;
    mutable std::mutex m_timer_mutex;
    std::condition_variable m_timer_cond;
    std::shared_ptr<Task> m_task;
};

} // namespace core
} // namespace aiotek