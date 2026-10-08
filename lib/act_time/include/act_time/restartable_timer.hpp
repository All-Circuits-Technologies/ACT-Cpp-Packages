// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/logger/abs_logger.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

namespace act::time
{

/**
 * @brief A restartable timer class that allows starting, stopping, and restarting a timer with
 * a specified duration. The timer can be configured to be periodic or one-shot.
 * @note The logger instances must outlive the timer instance.
 * @note The timer runs in its own thread to avoid blocking the main thread. The thread is
 *       started with the first start of the timer, and terminated when the timer is destroyed.
 * @note The timer state is a deadline guarded by a mutex: the methods update it, and the timer
 *       thread decides to expire on the current deadline, under the same mutex. A restart or a
 *       stop therefore always applies to the next expiry, and the state read is always the one
 *       left by the last call.
 * @note The callback runs in the timer thread. When the timer is also restarted or stopped
 *       from other threads, see StartId.
 */
class RestartableTimer
{
  public:
    /**
     * @brief Identifier of a start of the timer, given to the callback
     * @details The timer decides to expire under its lock, then calls the callback without it,
     *          so that the callback can restart or stop the timer. When another thread calls
     *          startOrRestart() or stop() in between, the callback still comes, for the expiry
     *          of the start made before that call:
     * @verbatim
       timer thread                       other thread
       ------------                       ------------
       expiry of start 1 decided
                                          startOrRestart() -> start 2
       callback(1)
       @endverbatim
     *          A callback whose behavior depends on what the other threads asked compares the
     *          identifier it receives with the one startOrRestart() returned last, and ignores
     *          the expiry of an older start: for instance an action postponed by the other
     *          thread must not run now, start 2 calls back on its own deadline.
     *
     *          The identifier is of no use when the timer is only started, restarted or stopped
     *          from its own callback, or when the callback does the same thing for every expiry:
     *          such a callback ignores it.
     * @note Each start gets the identifier of the previous start plus one, and the identifier
     *       after the greatest value is 0: an identifier can therefore be smaller than the
     *       previous one, and identifiers are only meant to be compared for equality.
     */
    using StartId = unsigned int;

  public:
    /**
     * @brief Constructor
     * @warning The logger instances must outlive the timer instance.
     * @param durationMs The duration in milliseconds after which the timer should expire
     * @param logger The logger instance to use for logging
     * @param callback The callback function to be called when the timer expires, with the
     *                 identifier of the start the expiry comes from (the same one at every
     *                 period of a periodic timer); see StartId for when it matters
     * @param periodic Flag indicating whether the timer should be periodic. If true, the timer
     *                 will restart automatically after expiring.
     *                 If false, the timer will stop after expiring once.
     * @param startImmediately Flag indicating whether the timer should start immediately upon
     *                         construction
     */
    explicit RestartableTimer(const act::foundation::AbsLogger &logger,
                              unsigned int durationMs,
                              const std::function<void(StartId)> &callback,
                              bool periodic = false,
                              bool startImmediately = false);

    /**
     * @brief Destructor
     * @note Waits for the callback if it is running; it must therefore not be called from the
     *       callback.
     */
    virtual ~RestartableTimer();

  public:
    /**
     * @brief Start or restart the timer with its current duration.
     * @return The identifier of this start, given to the callback when it expires
     */
    StartId startOrRestart();

    /**
     * @brief Start or restart the timer with the specified duration.
     * @param durationMs The duration in milliseconds after which the timer should expire, kept
     *                   as the duration of the timer
     * @return The identifier of this start, given to the callback when it expires
     */
    StartId startOrRestart(unsigned int durationMs);

    /**
     * @brief Stop the timer if it is running.
     * @note A callback already called because the timer expired just before may still be
     *       running when stop() returns.
     */
    void stop();

    /**
     * @brief Check if the timer is currently running.
     * @note A one-shot timer stops running when it expires, before its callback is called.
     * @return true if the timer is running, false otherwise.
     */
    [[nodiscard]] bool isRunning() const;

    /**
     * @brief Check if the timer is periodic.
     * @return true if the timer is periodic, false otherwise.
     */
    [[nodiscard]] bool isPeriodic() const
    {
        return m_periodic.load();
    }

    /**
     * @brief Get the duration of the timer.
     * @return The duration in milliseconds.
     */
    [[nodiscard]] unsigned int getDurationMs() const
    {
        return m_durationMs.load();
    }

    /**
     * @brief Set the duration of the timer.
     * @note If the timer is currently running, the new duration will take effect on the start
     * of the next period.
     * @param durationMs The new duration in milliseconds.
     */
    void setDurationMs(unsigned int durationMs);

    /**
     * @brief Set whether the timer is periodic.
     * @note If the timer is currently running, the new periodic setting will take effect on the
     *       end of the current period.
     * @param periodic Flag indicating whether the timer should be periodic.
     */
    void setPeriodic(bool periodic);

  private:
    /** @brief Clock the deadlines are measured with */
    using Clock = std::chrono::steady_clock;

  private:
    /**
     * @brief Start the timer to expire after its current duration from now.
     * @note The caller holds m_timerMutex.
     * @return The identifier of this start
     */
    StartId startLocked();

    /**
     * @brief The function executed by the timer thread: wait for the deadline and expire.
     */
    void runTimerThread();

  private:
    /** @brief Logger instance */
    const act::foundation::AbsLogger &m_logger;

    /** @brief Callback function to be called when the timer expires */
    std::function<void(StartId)> m_callback;

    /** @brief Mutex guarding the deadline, the start identifier, the termination and the thread */
    mutable std::mutex m_timerMutex;

    /** @brief Condition variable waking the timer thread when the deadline changes */
    std::condition_variable m_timerCv;

    /** @brief The timer thread, started with the first start of the timer */
    std::thread m_timerThread;

    /** @brief The time the timer expires at; empty while the timer is not running */
    std::optional<Clock::time_point> m_deadline;

    /** @brief The identifier of the last start */
    StartId m_startId{0};

    /** @brief Flag asking the timer thread to terminate */
    bool m_terminate{false};

    /** @brief Flag indicating whether the timer is periodic */
    std::atomic<bool> m_periodic{false};

    /** @brief The duration of the timer in milliseconds */
    std::atomic<unsigned int> m_durationMs{0};
};

} // namespace act::time
