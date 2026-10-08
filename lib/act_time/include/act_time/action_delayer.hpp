// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_time/restartable_timer.hpp"

#include <chrono>
#include <functional>
#include <mutex>
#include <optional>

namespace act::time
{

/**
 * @brief This helper allows to delay the execution of an action by a specified delay, with an
 *        optional maximum delay.
 * @note If the action is restarted with startOrRestart() before the delay expires,
 *       all delays are restarted. With startOrDelay(), only the delay is restarted.
 * @note The logger instances must outlive the ActionDelayer instance.
 * @note A single one-shot timer runs the action: it is started with the delay, shortened so
 *       that it never goes beyond the maximum delay. Only the expiry of the last start runs the
 *       action, so a start, a delay or a stop made while an expiry is being handled wins.
 * @attention The callback is called in the context of the timer thread.
 */
class ActionDelayer
{
  public:
    /**
     * @brief Constructor
     * @warning The logger instances must outlive the ActionDelayer instance.
     * @param logger The logger instance to use for logging
     * @param delayMs The delay in milliseconds after which the action should be executed
     * @param callback The callback function to be called when the delay expires
     * @param maxDelayMs Optional maximum delay in milliseconds after which the action should be
     *                   executed even if the delay is restarted multiple times. If not
     * provided, there is no maximum delay and the action will be executed only after the delay
     * expires without being restarted.
     * @param startImmediately Flag indicating whether the delay should start immediately upon
     *                         construction
     */
    explicit ActionDelayer(const act::foundation::AbsLogger &logger,
                           unsigned int delayMs,
                           const std::function<void()> &callback,
                           std::optional<unsigned int> maxDelayMs = std::nullopt,
                           bool startImmediately = false);

    /**
     * @brief Destructor
     * @note Waits for the action if it is running; it must therefore not be called from the
     *       action.
     */
    virtual ~ActionDelayer();

  public:
    /**
     * @brief Start or restart the delay. If the delay is already running, it will be restarted,
     *        same for the maximum delay if it was provided.
     */
    void startOrRestart();

    /**
     * @brief Start the delay if it is not already running. If the delay is already running, it
     * will be restarted, but not the maximum delay timer if it was provided.
     */
    void startOrDelay();

    /**
     * @brief Stop the delay. If the delay is not running, this method does nothing.
     * @note An action already started because the delay expired just before may still be
     *       running when stop() returns.
     */
    void stop();

    /**
     * @brief Check if the delay is currently running.
     * @note The delay stops running when its action is about to run.
     * @return true if the delay is running, false otherwise.
     */
    [[nodiscard]] bool isRunning() const;

  private:
    /** @brief Clock the maximum delay is measured with */
    using Clock = std::chrono::steady_clock;

  private:
    /**
     * @brief Start the delay, and the maximum delay if it was provided.
     * @note The caller holds m_actionMutex.
     * @param now The current time
     */
    void startLocked(Clock::time_point now);

    /**
     * @brief Get the delay to start the timer with: the delay, shortened to end at the latest
     *        on the maximum delay if it was provided.
     * @note The caller holds m_actionMutex.
     * @param now The current time
     * @return The delay in milliseconds
     */
    [[nodiscard]] unsigned int getCappedDelayMsLocked(Clock::time_point now) const;

    /**
     * @brief Called by the timer when a start expires: runs the action if it is the last start
     *        and the delay was not stopped meanwhile.
     * @param start The identifier of the expiring start
     */
    void onTimerExpired(RestartableTimer::StartId start);

  private:
    /** @brief Logger instance */
    const act::foundation::AbsLogger &m_logger;

    /** @brief Callback function to be called when the delay expires */
    std::function<void()> m_callback;

    /** @brief The delay in milliseconds after which the action is executed */
    unsigned int m_delayMs;

    /** @brief The optional maximum delay in milliseconds */
    std::optional<unsigned int> m_maxDelayMs;

    /** @brief Mutex guarding the state of the delay */
    mutable std::mutex m_actionMutex;

    /** @brief Flag indicating whether the delay is running */
    bool m_isRunning{false};

    /** @brief The identifier of the last start of the timer */
    RestartableTimer::StartId m_currentStart{0};

    /** @brief The time the maximum delay ends at, when it was provided and the delay started */
    std::optional<Clock::time_point> m_maxDeadline;

    /**
     * @brief The one-shot timer running the action
     * @note Declared last, so that it is destroyed first: its thread, which calls back this
     *       instance, is joined before the rest of the instance goes.
     */
    RestartableTimer m_timer;
};

} // namespace act::time
