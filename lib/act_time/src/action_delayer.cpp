// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_time/action_delayer.hpp"

#include "act_time/restartable_timer.hpp"

namespace act::time
{

ActionDelayer::ActionDelayer(const act::foundation::AbsLogger &logger,
                             unsigned int delayMs,
                             const std::function<void()> &callback,
                             std::optional<unsigned int> maxDelayMs,
                             bool startImmediately)
    : m_logger{logger},
      m_callback{callback},
      m_isRunning{false}
{
    m_delayRestartableTimer = new RestartableTimer(
        logger,
        delayMs,
        [this](RestartableTimer::StartId) { internalDelayTimerCallback(); },
        false,
        false);

    if (maxDelayMs.has_value())
    {
        m_maxDelayRestartableTimer = new RestartableTimer(
            logger,
            maxDelayMs.value(),
            [this](RestartableTimer::StartId) { internalMaxDelayTimerCallback(); },
            false,
            false);
    }

    if (startImmediately)
    {
        startOrRestart();
    }
}

ActionDelayer::~ActionDelayer()
{
    delete m_delayRestartableTimer;
    delete m_maxDelayRestartableTimer;
}

void ActionDelayer::startOrRestart()
{
    std::lock_guard<std::mutex> lock(m_actionMutex);
    startOrRestartProcess();
}

void ActionDelayer::startOrDelay()
{
    std::lock_guard<std::mutex> lock(m_actionMutex);

    if (!m_isRunning)
    {
        // We start the timers if they were not already running
        startOrRestartProcess();
        return;
    }

    // We delay the action by restarting the delay timer, but we do not restart the max delay
    // timer to keep the original max delay time from the first start
    m_delayRestartableTimer->startOrRestart();
}

void ActionDelayer::stop()
{
    std::lock_guard<std::mutex> lock(m_actionMutex);

    if (m_maxDelayRestartableTimer != nullptr)
    {
        m_maxDelayRestartableTimer->stop();
    }
    m_delayRestartableTimer->stop();
    m_isRunning = false;
}

bool ActionDelayer::isRunning()
{
    std::lock_guard<std::mutex> lock(m_actionMutex);
    return m_isRunning;
}

void ActionDelayer::startOrRestartProcess()
{
    if (m_maxDelayRestartableTimer != nullptr)
    {
        m_maxDelayRestartableTimer->startOrRestart();
    }
    m_delayRestartableTimer->startOrRestart();
    m_isRunning = true;
}

void ActionDelayer::internalDelayTimerCallback()
{
    std::unique_lock<std::mutex> lock(m_actionMutex);

    if (!m_isRunning)
    {
        // The timer callback can be called after stop() is called, in this case we do nothing
        return;
    }

    if (m_maxDelayRestartableTimer != nullptr)
    {
        m_maxDelayRestartableTimer->stop();
    }
    m_isRunning = false;

    lock.unlock();

    m_callback();
}

void ActionDelayer::internalMaxDelayTimerCallback()
{
    std::unique_lock<std::mutex> lock(m_actionMutex);

    if (!m_isRunning)
    {
        // The timer callback can be called after stop() is called, in this case we do nothing
        return;
    }

    m_delayRestartableTimer->stop();

    m_isRunning = false;

    lock.unlock();

    m_callback();
}

} // namespace act::time
