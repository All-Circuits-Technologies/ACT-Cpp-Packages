// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_logger/helpers/logger_helper.hpp"

#include <exception>

// These macros return from the caller, which no function can do
// NOLINTBEGIN(cppcoreguidelines-macro-usage)

/**
 * @brief Macros to help handling database not opened and exceptions
 * @param caller The caller function name
 * @param logger A pointer to the logger helper to use
 * @param xxx The value to return in case of error
 */
#define XXX_IF_NO_DB(caller, logger, xxx)                                                          \
    if (!isOpened())                                                                               \
    {                                                                                              \
        (logger)->errorStream() << "Database is not opened, cannot perform operation: "            \
                                << #caller;                                                        \
        return xxx;                                                                                \
    }

/**
 * @brief Macros to help handling database exceptions
 * @param name The name of the operation being performed
 * @param cmd The command to execute
 * @param logger A pointer to the logger helper to use
 * @param xxx The value to return in case of error
 */
#define XXX_IF_THROW(name, cmd, logger, xxx)                                                       \
    try                                                                                            \
    {                                                                                              \
        cmd;                                                                                       \
    }                                                                                              \
    catch (const std::exception &e)                                                                \
    {                                                                                              \
        (logger)->errorStream() << "Error occurred: " << e.what() << " upon " #name;               \
        return xxx;                                                                                \
    }

/**
 * @brief Macros shortcuts to return false if database not opened
 * @param caller The caller function name
 * @param logger A pointer to the logger helper to use
 */
#define FALSE_IF_NO_DB(caller, logger) XXX_IF_NO_DB(caller, logger, false);

/**
 * @brief Macros shortcuts to return false upon exception
 * @param name The name of the operation being performed
 * @param logger A pointer to the logger helper to use
 * @param cmd The command to execute
 */
#define FALSE_IF_THROW(name, logger, cmd) XXX_IF_THROW(name, cmd, logger, false);

/**
 * @brief Macros shortcuts to return empty if database not opened
 * @param caller The caller function name
 * @param logger A pointer to the logger helper to use
 */
#define EMPTY_IF_NO_DB(caller, logger) XXX_IF_NO_DB(caller, logger, {});

/**
 * @brief Macros shortcuts to return empty upon exception
 * @param name The name of the operation being performed
 * @param logger A pointer to the logger helper to use
 * @param cmd The command to execute
 */
#define EMPTY_IF_THROW(name, logger, cmd) XXX_IF_THROW(name, cmd, logger, {});

/**
 * @brief Macros shortcuts to return zero if database not opened
 * @param caller The caller function name
 * @param logger A pointer to the logger helper to use
 */
#define ZERO_IF_NO_DB(caller, logger) XXX_IF_NO_DB(caller, logger, 0);

/**
 * @brief Macros shortcuts to return zero upon exception
 * @param name The name of the operation being performed
 * @param logger A pointer to the logger helper to use
 * @param cmd The command to execute
 */
#define ZERO_IF_THROW(name, logger, cmd) XXX_IF_THROW(name, cmd, logger, 0);

// NOLINTEND(cppcoreguidelines-macro-usage)
