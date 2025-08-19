#include "aiotek_timer.hpp"
#include "aiotek_logger.hpp"
#include <iostream>
#include <sys/resource.h>

namespace aiotek {
namespace core {

static void LogMemoryUsage(const std::string& prefix)
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    AIOTEK_LOG_DEBUG(prefix << ": Memory usage: " << usage.ru_maxrss << "KB");
}

Timer::~Timer()
{
    Deinitialize();
}

void Timer::Initialize(const std::string& name, int id, TimerType type, std::chrono::milliseconds period, TimerCallback callback, void* user_data)
{
    m_name = name;
    m_id = id;
    m_type = type;
    m_period = period;
    m_callback = callback;
    m_user_data = user_data;

    AIOTEK_LOG_INFO("Timer: " << name << "id: " << id << "type: " << (type == TimerType::ONESHOT ? "ONE_SHOT" : "PERIODIC")
                              << "period: " << period.count() << "ms");

    LogMemoryUsage("Timer");
}

void Timer::Deinitialize()
{
    Delete();
    Stop();
}

void Timer::Start(std::chrono::milliseconds start_time)
{
    std::chrono::milliseconds effective_time =
        (m_remaining_time > std::chrono::milliseconds(0)) ? m_remaining_time : (start_time > std::chrono::milliseconds(0) ? start_time : m_period);
    m_deadline = std::chrono::steady_clock::now() + effective_time;
    m_remaining_time = std::chrono::milliseconds(0);
    m_activated = true;

    TimerManager::GetInstance().AddTimer(this);

    AIOTEK_LOG_INFO("Timer: Started " << m_name << "id: " << m_id << "start_time: " << effective_time.count() << "ms");
}

void Timer::Stop()
{
    if (m_activated) {
        auto now = std::chrono::steady_clock::now();
        if (now < m_deadline) {
            m_remaining_time = std::chrono::duration_cast<std::chrono::milliseconds>(m_deadline - now);
        } else {
            m_remaining_time = std::chrono::milliseconds(0);
        }
    }

    TimerManager::GetInstance().RemoveTimer(this);

    m_activated = false;
}

void Timer::Pause()
{
    Stop();
    AIOTEK_LOG_INFO("Timer: Paused " << m_name);
}

void Timer::Resume()
{
    Start(std::chrono::milliseconds(0));
    AIOTEK_LOG_INFO("Timer: Resumed " << m_name);
}

void Timer::SetPeriod(std::chrono::milliseconds new_period)
{
    m_period = new_period;

    if (m_activated) {
        TimerManager::GetInstance().UpdateDeadline(this, new_period);
    }

    AIOTEK_LOG_INFO("Timer: Set new period for " << m_name << "new_period " << new_period.count() << "ms");
}

void Timer::Delete()
{
    TimerManager::GetInstance().RemoveTimer(this);

    m_activated = false;

    if (m_callback) {
        m_callback(this, m_user_data);
    }
}

bool Timer::IsActivated() const
{
    return m_activated && TimerManager::GetInstance().IsTimerInList(this);
}

TimerManager& TimerManager::GetInstance()
{
    static TimerManager instance;
    return instance;
}

TimerManager::TimerManager()
{
    m_task = std::make_shared<Task>("TimerManager", 0, [](Task&) {});
}

TimerManager::~TimerManager()
{
    Stop();
}

void TimerManager::Start()
{
    m_task->Start();
}

void TimerManager::Stop()
{
    std::lock_guard<std::mutex> lock(m_timer_mutex);
    m_timer_list.clear();
    m_task->Stop();
    m_timer_cond.notify_one();
}

void TimerManager::AddTimer(Timer* timer)
{
    std::lock_guard<std::mutex> lock(m_timer_mutex);
    auto& timer_list = m_timer_list;
    auto it = timer_list.begin();
    for (; it != timer_list.end(); ++it) {
        if (timer->m_deadline < (*it)->m_deadline) {
            break;
        }
    }
    timer_list.insert(it, timer);
    m_timer_cond.notify_one();
}

void TimerManager::RemoveTimer(Timer* timer)
{
    std::lock_guard<std::mutex> lock(m_timer_mutex);
    m_timer_list.remove_if([timer](const Timer* t) { return t == timer; });
    m_timer_cond.notify_one();
}

void TimerManager::UpdateDeadline(Timer* timer, std::chrono::milliseconds new_deadline)
{
    std::lock_guard<std::mutex> lock(m_timer_mutex);
    RemoveTimer(timer);
    timer->m_deadline = std::chrono::steady_clock::now() + new_deadline;
    auto& timer_list = m_timer_list;
    auto it = timer_list.begin();
    for (; it != timer_list.end(); ++it) {
        if (timer->m_deadline < (*it)->m_deadline) {
            break;
        }
    }
    timer_list.insert(it, timer);
    m_timer_cond.notify_one();
}

bool TimerManager::IsTimerInList(const Timer* timer) const
{
    std::lock_guard<std::mutex> lock(m_timer_mutex);
    return std::find(m_timer_list.begin(), m_timer_list.end(), timer) != m_timer_list.end();
}

void TimerManager::ThreadTimerHandler(Task& task)
{
    while (task.IsOperation()) {
        if (task.IsSuspended()) {
            AIOTEK_LOG_DEBUG("TimerManager: Suspended");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        std::unique_lock<std::mutex> lock(m_timer_mutex);
        if (m_timer_list.empty()) {
            m_timer_cond.wait(lock);
            continue;
        }

        auto* timer = m_timer_list.front();
        auto now = std::chrono::steady_clock::now();
        if (now < timer->m_deadline) {
            m_timer_cond.wait_until(lock, timer->m_deadline);
            continue;
        }

        m_timer_list.pop_front();
        lock.unlock();

        if (timer->m_callback) {
            timer->m_callback(timer, timer->m_user_data);
        }

        if (timer->m_type == Timer::TimerType::PERIODIC && timer->m_activated) {
            timer->Start(timer->m_period);
        } else {
            timer->m_activated = false;
        }
    }
}

} // namespace core
} // namespace aiotek