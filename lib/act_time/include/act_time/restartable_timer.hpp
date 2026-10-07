// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/logger/abs_logger.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

namespace act::time
{

/**
 * @brief A restartable timer class that allows starting, stopping, and restarting a timer with
 * a specified duration. The timer can be configured to be periodic or one-shot.
 * @note The logger instances must outlive the timer instance.
 * @note The timer runs in its own thread to avoid blocking the main thread. The thread is
 *       terminated when the timer is destroyed.
 */
class RestartableTimer
{
  private:
    /** @brief Requested timer state */
    enum class RequestedTimerState : int
    {
        NONE = 0,
        RESTART,
        STOP,
        TERMINATE,
    };

  public:
    /**
     * @brief Constructor
     * @warning The logger instances must outlive the timer instance.
     * @param durationMs The duration in milliseconds after which the timer should expire
     * @param logger The logger instance to use for logging
     * @param callback The callback function to be called when the timer expires
     * @param periodic Flag indicating whether the timer should be periodic. If true, the timer
     *                 will restart automatically after expiring.
     *                 If false, the timer will stop after expiring once.
     * @param startImmediately Flag indicating whether the timer should start immediately upon
     *                         construction
     */
    explicit RestartableTimer(const act::foundation::AbsLogger &logger,
                              unsigned int durationMs,
                              const std::function<void()> &callback,
                              bool periodic = false,
                              bool startImmediately = false);

    /// @brief Destructor
    virtual ~RestartableTimer();

  public:
    /**
     * @brief Start or restart the timer with the specified duration.
     */
    void startOrRestart()
    {
        startOrRestartProcess(nullptr);
    }

    /**
     * @brief Start or restart the timer with the specified duration.
     * @param durationMs The duration in milliseconds after which the timer should expire
     */
    void startOrRestart(unsigned int durationMs)
    {
        startOrRestartProcess(&durationMs);
    }

    /**
     * @brief Stop the timer if it is running.
     */
    void stop();

    /**
     * @brief Check if the timer is currently running.
     * @return true if the timer is running, false otherwise.
     */
    [[nodiscard]] bool isRunning() const
    {
        return m_running.load();
    }

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
    /**
     * @brief Start or restart the timer with the specified duration.
     * @param durationMsPtr Pointer to the duration in milliseconds after which the timer should
     *                     expire. If nullptr, the current duration will be used.
     */
    void startOrRestartProcess(const unsigned int *durationMsPtr);

    /**
     * @brief Set the requested timer state.
     * @note This is the method used by the timer to communicate with the timer thread.
     * @note If the current state is TERMINATE, the state given will not be applied.
     * @param state The requested timer state.
     */
    void setRequestedState(RequestedTimerState state);

  private:
    /**
     * @brief The function executed by the timer thread.
     * @param timerInstance Pointer to the RestartableTimer instance.
     */
    static void TimerThreadFunction(RestartableTimer *timerInstance);

  private:
    /** @brief Logger instance */
    const act::foundation::AbsLogger &m_logger;

    /** @brief Callback function to be called when the timer expires */
    std::function<void()> m_callback;

    /** @brief Mutex to protect timer operations */
    std::mutex m_timerMutex;

    /** @brief Condition variable to signal timer thread */
    std::condition_variable m_timerCv;

    /** @brief The timer thread */
    std::thread *m_timerThread{nullptr};

    /** @brief Flag indicating whether the timer is periodic */
    std::atomic<bool> m_periodic{false};

    /** @brief Flag indicating whether the timer is running */
    std::atomic<bool> m_running{false};

    /**
     * @brief Flag to request stopping the timer
     * @note Even if we use a condition variable and a mutex to protect the timer state,
     *       we use an atomic variable here to be faster in the destructor, where we need to
     *       set the state to TERMINATE and notify the timer thread.
     */
    std::atomic<RequestedTimerState> m_requestedState{RequestedTimerState::NONE};

    /** @brief The duration of the timer in milliseconds */
    std::atomic<unsigned int> m_durationMs{0};
};

} // namespace act::time
