// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_time/action_delayer.hpp"
#include "act_foundation/logger/abs_logger.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <mutex>
#include <optional>

namespace act::time
{

ActionDelayer::ActionDelayer(const act::foundation::AbsLogger &logger,
                             unsigned int delayMs,
                             const std::function<void()> &callback,
                             std::optional<unsigned int> maxDelayMs,
                             bool startImmediately)
    : m_logger{logger},
      m_callback{callback},
      m_delayMs{delayMs},
      m_maxDelayMs{maxDelayMs},
      m_timer{logger, delayMs, [this](RestartableTimer::StartId start) { onTimerExpired(start); }}
{
    if (startImmediately)
    {
        startOrRestart();
    }
}

ActionDelayer::~ActionDelayer() = default;

void ActionDelayer::startOrRestart()
{
    const std::scoped_lock lock(m_actionMutex);
    startLocked(Clock::now());
}

void ActionDelayer::startOrDelay()
{
    const std::scoped_lock lock(m_actionMutex);
    const auto now = Clock::now();

    if (!m_isRunning)
    {
        startLocked(now);
        return;
    }

    // Delaying keeps the maximum delay of the running start
    m_currentStart = m_timer.startOrRestart(getCappedDelayMsLocked(now));
}

void ActionDelayer::stop()
{
    const std::scoped_lock lock(m_actionMutex);
    m_isRunning = false;
    m_timer.stop();
}

bool ActionDelayer::isRunning() const
{
    const std::scoped_lock lock(m_actionMutex);
    return m_isRunning;
}

void ActionDelayer::startLocked(Clock::time_point now)
{
    m_maxDeadline.reset();
    if (m_maxDelayMs.has_value())
    {
        m_maxDeadline = now + std::chrono::milliseconds(*m_maxDelayMs);
    }

    m_isRunning = true;
    m_currentStart = m_timer.startOrRestart(getCappedDelayMsLocked(now));
}

unsigned int ActionDelayer::getCappedDelayMsLocked(Clock::time_point now) const
{
    if (!m_maxDeadline.has_value())
    {
        return m_delayMs;
    }

    // Rounded up, so that the action never runs before the end of the maximum delay
    const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(
        std::max(*m_maxDeadline - now, Clock::duration::zero()));
    return std::min(m_delayMs, static_cast<unsigned int>(remaining.count()));
}

void ActionDelayer::onTimerExpired(RestartableTimer::StartId start)
{
    {
        const std::scoped_lock lock(m_actionMutex);
        if (!m_isRunning || start != m_currentStart)
        {
            // Stopped, or started again while this expiry was on its way: the last start rules
            return;
        }
        m_isRunning = false;
    }

    m_callback();
}

} // namespace act::time
