// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/logger/abs_logger.hpp"

#include <utility>

namespace act::foundation
{

/**
 * @brief Object executing a function when going out of scope
 *
 * @note C++23 appears to have such thing named std::experimental::scope_exit
 */
template <typename F>
class Finally
{
    /* ## Types */
    /* ## Constructors */
  public:
    /**
     * @brief Constructor forwarding arguments to the value's constructor
     * @param func Function to execute upon destruction
     * @param logger Logger to log errors occurring during function execution
     * @note Throwing from inside a destructor is strictly forbidden in C++. As a safety measure,
     *       the std::exception thrown by the function are caught and logged as errors with
     *       @p logger. Any other exception leaves the destructor, which is noexcept, so the
     *       program terminates.
     */
    explicit Finally(F &&func, act::foundation::AbsLogger &logger)
        : m_enabled(true),
          m_func(std::forward<F>(func)),
          m_logger(logger)
    {
        // Empty
    }

    /**
     * @brief Default destructor
     */
    ~Finally()
    {
        if (m_enabled)
        {
            try
            {
                m_func();
            }
            catch (const std::exception &e)
            {
                // Throwing from a destructor is strictly denied; only std::exception can be
                // logged, any other exception terminates the program

                m_logger.errorStream() << "Exception caught in Finally destructor: " << e.what();
            }
        }
    }

    /* ## Methods (members, then non-members) */
  public:
    /**
     * @brief Disable the execution of the function at destruction
     * @note Once disabled, cannot be re-enabled
     */
    void cancel()
    {
        m_enabled = false;
    }

    /* ## Constants */

    /* ## Data members */
  private:
    /**
     * @brief Should function be called upon destruction
     * @note True a start, false once cancelled
     */
    bool m_enabled;

    /**
     * @brief Function to execute
     * @note This function should better not throw exceptions
     */
    F m_func;

    /**
     * @brief Logger
     */
    act::foundation::AbsLogger &m_logger;
};

} // namespace act::foundation
