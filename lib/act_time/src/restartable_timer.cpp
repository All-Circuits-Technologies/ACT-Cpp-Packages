// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_time/restartable_timer.hpp"

namespace act::time
{

RestartableTimer::RestartableTimer(const act::foundation::AbsLogger &logger,
                                   unsigned int durationMs,
                                   const std::function<void(StartId)> &callback,
                                   bool periodic,
                                   bool startImmediately)
    : m_logger{logger},
      m_callback{callback},
      m_periodic{periodic},
      m_durationMs{durationMs}
{
    if (startImmediately)
    {
        startOrRestart();
    }
}

RestartableTimer::~RestartableTimer()
{
    {
        const std::scoped_lock lock(m_timerMutex);
        m_terminate = true;
    }
    m_timerCv.notify_all();

    if (m_timerThread.joinable())
    {
        m_timerThread.join();
    }
}

RestartableTimer::StartId RestartableTimer::startOrRestart()
{
    const std::scoped_lock lock(m_timerMutex);
    return startLocked();
}

RestartableTimer::StartId RestartableTimer::startOrRestart(unsigned int durationMs)
{
    const std::scoped_lock lock(m_timerMutex);
    m_durationMs.store(durationMs);
    return startLocked();
}

void RestartableTimer::stop()
{
    {
        const std::scoped_lock lock(m_timerMutex);
        m_deadline.reset();
    }
    m_timerCv.notify_all();
}

bool RestartableTimer::isRunning() const
{
    const std::scoped_lock lock(m_timerMutex);
    return m_deadline.has_value();
}

void RestartableTimer::setDurationMs(unsigned int durationMs)
{
    m_durationMs.store(durationMs);
}

void RestartableTimer::setPeriodic(bool periodic)
{
    m_periodic.store(periodic);
}

RestartableTimer::StartId RestartableTimer::startLocked()
{
    m_deadline = Clock::now() + std::chrono::milliseconds(m_durationMs.load());

    // StartId is unsigned: incrementing the greatest value wraps around to 0, which the language
    // defines. The identifiers are only compared for equality, so going back to 0 is harmless.
    ++m_startId;

    if (!m_timerThread.joinable())
    {
        m_timerThread = std::thread(&RestartableTimer::runTimerThread, this);
    }

    m_timerCv.notify_all();
    return m_startId;
}

void RestartableTimer::runTimerThread()
{
    std::unique_lock<std::mutex> lock(m_timerMutex);
    while (!m_terminate)
    {
        if (!m_deadline.has_value())
        {
            m_timerCv.wait(lock);
            continue;
        }

        const auto now = Clock::now();
        if (now < *m_deadline)
        {
            m_timerCv.wait_until(lock, *m_deadline);
            continue;
        }

        // The timer expires on the deadline read under the lock: a restart or a stop made
        // before it has already moved or removed it
        const StartId expiringStart = m_startId;
        if (m_periodic.load())
        {
            m_deadline = now + std::chrono::milliseconds(m_durationMs.load());
        }
        else
        {
            m_deadline.reset();
        }

        // The callback runs without the lock, so that it can restart or stop the timer
        lock.unlock();
        m_callback(expiringStart);
        lock.lock();
    }
}

} // namespace act::time
