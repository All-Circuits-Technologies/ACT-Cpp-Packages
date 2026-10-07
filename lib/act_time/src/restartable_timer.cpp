// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_time/restartable_timer.hpp"

namespace act::time
{

RestartableTimer::RestartableTimer(const act::foundation::AbsLogger &logger,
                                   unsigned int durationMs,
                                   const std::function<void()> &callback,
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
    if (m_timerThread != nullptr)
    {
        m_requestedState.store(RequestedTimerState::TERMINATE);
        m_timerCv.notify_all();
        m_timerThread->join();
        delete m_timerThread;
    }
}

void RestartableTimer::stop()
{
    std::lock_guard<std::mutex> lock(m_timerMutex);
    setRequestedState(RequestedTimerState::STOP);
}

void RestartableTimer::setDurationMs(unsigned int durationMs)
{
    std::lock_guard<std::mutex> lock(m_timerMutex);
    m_durationMs.store(durationMs);
}

void RestartableTimer::setPeriodic(bool periodic)
{
    std::lock_guard<std::mutex> lock(m_timerMutex);
    m_periodic.store(periodic);
}

void RestartableTimer::startOrRestartProcess(const unsigned int *durationMsPtr)
{
    std::lock_guard<std::mutex> lock(m_timerMutex);

    if (durationMsPtr != nullptr)
    {
        m_durationMs.store(*durationMsPtr);
    }

    setRequestedState(RequestedTimerState::RESTART);

    if (m_timerThread == nullptr)
    {
        m_timerThread = new std::thread(TimerThreadFunction, this);
    }
}

void RestartableTimer::setRequestedState(RequestedTimerState state)
{
    auto requestedState = m_requestedState.load();
    if (requestedState == RequestedTimerState::TERMINATE)
    {
        // The timer is terminating, do not change the state
        return;
    }

    m_requestedState.store(state);
    m_timerCv.notify_all();
}

void RestartableTimer::TimerThreadFunction(RestartableTimer *timerInstance)
{
    auto requestedState = timerInstance->m_requestedState.load();
    auto clockAtStart = std::chrono::steady_clock::now();
    auto durationMs = std::chrono::milliseconds(timerInstance->m_durationMs.load());
    while (requestedState != RequestedTimerState::TERMINATE)
    {
        if (requestedState == RequestedTimerState::RESTART)
        {
            timerInstance->m_requestedState.store(RequestedTimerState::NONE);
            timerInstance->m_running.store(true);
            durationMs = std::chrono::milliseconds(timerInstance->m_durationMs.load());
            clockAtStart = std::chrono::steady_clock::now();
        }
        else if (requestedState == RequestedTimerState::STOP)
        {
            timerInstance->m_requestedState.store(RequestedTimerState::NONE);
            timerInstance->m_running.store(false);
        }

        auto timeToWait = durationMs - (std::chrono::steady_clock::now() - clockAtStart);
        if (timeToWait <= std::chrono::milliseconds::zero() && timerInstance->m_running.load())
        {
            // Timer expired
            timerInstance->m_callback();

            if (timerInstance->m_periodic.load())
            {
                // Restart the timer
                clockAtStart = std::chrono::steady_clock::now();
            }
            else
            {
                // Stop the timer
                timerInstance->m_running.store(false);
            }

            // We reload the duration in case it was changed during the callback
            durationMs = std::chrono::milliseconds(timerInstance->m_durationMs.load());
        }
        else if (timerInstance->m_running.load())
        {
            // We wait for the timer to expire or a requested state change
            std::unique_lock<std::mutex> lock(timerInstance->m_timerMutex);
            timerInstance->m_timerCv.wait_for(lock, timeToWait, [timerInstance]() {
                return timerInstance->m_requestedState.load() != RequestedTimerState::NONE;
            });
        }
        else
        {
            // Timer is not running, wait for a requested state change
            std::unique_lock<std::mutex> lock(timerInstance->m_timerMutex);
            timerInstance->m_timerCv.wait(lock, [timerInstance]() {
                return timerInstance->m_requestedState.load() != RequestedTimerState::NONE;
            });
        }

        requestedState = timerInstance->m_requestedState.load();
    }
}

} // namespace act::time
