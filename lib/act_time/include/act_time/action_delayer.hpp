// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <functional>
#include <mutex>
#include <optional>

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

namespace act::time
{

class RestartableTimer;

/**
 * @brief This helper allows to delay the execution of an action by a specified delay, with an
 *        optional maximum delay.
 * @note If the action is restarted with startOrRestart() before the delay expires,
 *       all delays are restarted. With startOrDelay(), only the delay is restarted.
 * @note The logger instances must outlive the ActionDelayer instance.
 * @attention The callback is called in the context of the delay timer thread.
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
     */
    void stop();

    /**
     * @brief Check if the delay is currently running.
     * @return true if the delay is running, false otherwise.
     */
    [[nodiscard]] bool isRunning();

  private:
    /**
     * @brief Start or restart the delay. If the delay is already running, it will be restarted,
     *        same for the maximum delay if it was provided.
     * @note This method isn't protected by the mutex, it is the caller responsibility to call
     * it in a thread-safe way.
     */
    void startOrRestartProcess();

    /**
     * @brief Internal callback for the delay timer. This is the callback that will be called by
     * the delay timer when the delay expires. It will call the user-provided callback and stop
     * the delay.
     * @note Called in the context of the delay timer thread.
     */
    void internalDelayTimerCallback();

    /**
     * @brief Internal callback for the maximum delay timer. This is the callback that will be
     *        called by the maximum delay timer when the maximum delay expires. It will call the
     *        user-provided callback and stop the delay.
     * @note Called in the context of the maximum delay timer thread.
     */
    void internalMaxDelayTimerCallback();

  private:
    /** @brief Logger instance */
    const act::foundation::AbsLogger &m_logger;

    /** @brief Callback function to be called when the timer expires */
    std::function<void()> m_callback;

    /** @brief Mutex to protect timer operations */
    std::mutex m_actionMutex;

    /** @brief Flag indicating whether the delay is currently running */
    bool m_isRunning;

    /** @brief Timer for the delay */
    RestartableTimer *m_delayRestartableTimer{nullptr};

    /** @brief Optional timer for the maximum delay */
    RestartableTimer *m_maxDelayRestartableTimer{nullptr};
};

} // namespace act::time
