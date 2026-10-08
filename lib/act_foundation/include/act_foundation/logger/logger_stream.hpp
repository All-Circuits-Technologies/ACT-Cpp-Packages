// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/logger/logs_level.hpp"

#include <sstream>

namespace act::foundation
{

class AbsLogger;

/** @brief This class helps to log messages using stream syntax */
class LoggerStream
{
  public:
    /** @brief Constructor */
    explicit LoggerStream(LogsLevel::Enum level, const AbsLogger &logger);

    /** @brief Destructor */
    virtual ~LoggerStream();

  public:
    /** @brief Get the underlying string stream */
    std::ostringstream &getStream()
    {
        return m_stream;
    }

    /** @brief Overload of the stream operator */
    template <typename T>
    LoggerStream &operator<<(const T &value)
    {
        m_stream << value;
        return *this;
    }

  private:
    /** @brief The underlying string stream */
    std::ostringstream m_stream;

    /** @brief The logger used to log */
    const AbsLogger &m_logger;

    /** @brief The level of the log */
    LogsLevel::Enum m_level;
};

} // namespace act::foundation
