// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_logger/models/abs_logger.hpp"
#include "act_threading/types/reusable_thread_result.hpp"

#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

namespace act::threading
{

/**
 * @brief A reusable thread that can be started multiple times
 * @note If the thread is already running, starting it again will either wait for it to finish
 *       or return an ALREADY_RUNNING status, depending on the parameter passed to start().
 * @note The thread object is deleted when the ReusableThread is destroyed, after joining it if
 *       still running.
 * @note The class works with two threads: the working thread that runs the user function,
 *       and a waiting thread that waits for the working thread to finish before allowing a new
 *       start.
 *       Therefore, this class can be costly if the thread function is called frequently. Prefer
 * it if the task can only be started occasionally and if only one instance of the task should
 *       run at a time.
 */
class ReusableThread
{
  public:
    /**
     * @brief Constructor
     */
    explicit ReusableThread() = default;

    /**
     * @brief Destructor
     * @note Joins the thread if it's still running
     */
    virtual ~ReusableThread();

  public:
    /**
     * @brief Check if the thread is currently running
     * @return true if the thread is running, false otherwise
     */
    [[nodiscard]] bool isRunning() const;

    /**
     * @brief Start the thread with the given function and arguments
     * @tparam Func The function type
     * @tparam Args The argument types
     * @param waitToJoin Whether to wait for the previous thread to join if it's running. If
     * false, the method returns ALREADY_RUNNING.
     * @param logger The logger to use for logging
     * @param preconditionFunc The function that returns a value to send to the thread function.
     *                         If the function returns a nullopt, this means that an error
     * occurred and the thread will not be started. The value returned by the precondition
     * function is passed as the first argument to the thread function, before the other
     * arguments.
     * @param func The function to run in the thread
     * @param args The arguments to pass to the function
     */
    template <typename T, typename Func, typename... Args>
    ReusableThreadResult::Enum start(bool waitToJoin,
                                     const act::logger::AbsLogger &logger,
                                     const std::function<std::optional<T>()> &preconditionFunc,
                                     Func &&func,
                                     Args &&...args);

    /**
     * @brief Start the thread with the given function and arguments
     * @tparam Func The function type
     * @tparam Args The argument types
     * @param waitToJoin Whether to wait for the previous thread to join if it's running. If
     * false, the method returns ALREADY_RUNNING.
     * @param logger The logger to use for logging
     * @param func The function to run in the thread
     * @param args The arguments to pass to the function
     */
    template <typename Func, typename... Args>
    ReusableThreadResult::Enum start(bool waitToJoin,
                                     const act::logger::AbsLogger &logger,
                                     Func &&func,
                                     Args &&...args);

    /**
     * @brief Start the thread with the given function and arguments
     * @note This overload does not wait for the previous thread to join.
     * @tparam Func The function type
     * @tparam Args The argument types
     * @param logger The logger to use for logging
     * @param preconditionFunc The function that returns a value to send to the thread function.
     *                         If the function returns a nullopt, this means that an error
     * occurred and the thread will not be started. The value returned by the precondition
     * function is passed as the first argument to the thread function, before the other
     * arguments.
     * @param func The function to run in the thread
     * @param args The arguments to pass to the function
     */
    template <typename T, typename Func, typename... Args>
    ReusableThreadResult::Enum start(const act::logger::AbsLogger &logger,
                                     const std::function<std::optional<T>()> &preconditionFunc,
                                     Func &&func,
                                     Args &&...args);

    /**
     * @brief Start the thread with the given function and arguments
     * @note This overload does not wait for the previous thread to join and does not check any
     *       precondition.
     * @tparam Func The function type
     * @tparam Args The argument types
     * @param logger The logger to use for logging
     * @param func The function to run in the thread
     * @param args The arguments to pass to the function
     */
    template <typename Func, typename... Args>
    ReusableThreadResult::Enum start(const act::logger::AbsLogger &logger,
                                     Func &&func,
                                     Args &&...args);

  private:
    /**
     * @brief Start the thread with the given thread factory function
     * @param waitToJoin Whether to wait for the previous thread to join if it's running. If
     * false, the method returns ALREADY_RUNNING.
     * @param logger The logger to use for logging
     * @param threadFactory The function that creates and returns a pointer to the thread to run
     * @return The result of starting the thread
     */
    ReusableThreadResult::Enum startProcess(bool waitToJoin,
                                            const act::logger::AbsLogger &logger,
                                            const std::function<std::thread *()> &threadFactory);

  private:
    /**
     * @brief The function run by the waiting thread to wait for the reusable thread to finish
     * @param currentThread The reusable thread instance
     */
    static void WaitingThreadFunction(ReusableThread *currentThread);

  private:
    /** @brief The working thread */
    std::thread *m_workingThread{nullptr};

    /** @brief The thread that is waiting for the reusable thread to finish */
    std::thread *m_waitingThread{nullptr};

    /** @brief Atomic flag to indicate if the thread is running */
    std::atomic<bool> m_threadRunning{false};

    /** @brief Mutex to protect thread start, if it's asked from different thread */
    std::mutex m_startThreadMutex;
};

template <typename T, typename Func, typename... Args>
inline ReusableThreadResult::Enum ReusableThread::start(
    bool waitToJoin,
    const act::logger::AbsLogger &logger,
    const std::function<std::optional<T>()> &preconditionFunc,
    Func &&func,
    Args &&...args)
{
    return startProcess(waitToJoin,
                        logger,
                        [this, &logger, &preconditionFunc, &func, &args...]() -> std::thread * {
                            // Call precondition function to get the condition
                            auto conditionOpt = preconditionFunc();
                            if (!conditionOpt.has_value())
                            {
                                logger.warning("Precondition failed, the thread is not started");
                                return nullptr;
                            }

                            return new std::thread(std::forward<Func>(func),
                                                   conditionOpt.value(),
                                                   std::forward<Args>(args)...);
                        });
}

template <typename Func, typename... Args>
inline ReusableThreadResult::Enum ReusableThread::start(bool waitToJoin,
                                                        const act::logger::AbsLogger &logger,
                                                        Func &&func,
                                                        Args &&...args)
{
    return startProcess(waitToJoin, logger, [this, &func, &args...]() -> std::thread * {
        return new std::thread(std::forward<Func>(func), std::forward<Args>(args)...);
    });
}

template <typename T, typename Func, typename... Args>
inline ReusableThreadResult::Enum ReusableThread::start(
    const act::logger::AbsLogger &logger,
    const std::function<std::optional<T>()> &preconditionFunc,
    Func &&func,
    Args &&...args)
{
    return start(false,
                 logger,
                 preconditionFunc,
                 std::forward<Func>(func),
                 std::forward<Args>(args)...);
}

template <typename Func, typename... Args>
inline ReusableThreadResult::Enum ReusableThread::start(const act::logger::AbsLogger &logger,
                                                        Func &&func,
                                                        Args &&...args)
{
    return start(false, logger, std::forward<Func>(func), std::forward<Args>(args)...);
}

} // namespace act::threading
