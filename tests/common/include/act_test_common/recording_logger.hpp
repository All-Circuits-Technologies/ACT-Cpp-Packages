// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file recording_logger.hpp
 * @brief Logger double keeping every message it receives, so a test can check what was logged.
 */

#pragma once

#include "act_foundation/logger/abs_logger.hpp"
#include "act_foundation/logger/logger_stream.hpp"
#include "act_foundation/logger/logs_level.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace act::tests
{

/** @brief Logger recording every message instead of printing it */
class RecordingLogger : public act::foundation::AbsLogger
{
  public:
    /** @brief One recorded message */
    struct Entry
    {
        act::foundation::LogsLevel::Enum level;
        std::string message;
    };

  public:
    void log(act::foundation::LogsLevel::Enum level, const std::string &message) const override
    {
        m_entries.push_back({level, message});
    }

    [[nodiscard]] act::foundation::LoggerStream logStream(
        act::foundation::LogsLevel::Enum level) const override
    {
        return act::foundation::LoggerStream(level, *this);
    }

    void trace(const std::string &message) const override
    {
        log(act::foundation::LogsLevel::TRACE, message);
    }

    [[nodiscard]] act::foundation::LoggerStream traceStream() const override
    {
        return logStream(act::foundation::LogsLevel::TRACE);
    }

    void debug(const std::string &message) const override
    {
        log(act::foundation::LogsLevel::DEBG, message);
    }

    [[nodiscard]] act::foundation::LoggerStream debugStream() const override
    {
        return logStream(act::foundation::LogsLevel::DEBG);
    }

    void info(const std::string &message) const override
    {
        log(act::foundation::LogsLevel::INFO, message);
    }

    [[nodiscard]] act::foundation::LoggerStream infoStream() const override
    {
        return logStream(act::foundation::LogsLevel::INFO);
    }

    void warning(const std::string &message) const override
    {
        log(act::foundation::LogsLevel::WARNING, message);
    }

    [[nodiscard]] act::foundation::LoggerStream warningStream() const override
    {
        return logStream(act::foundation::LogsLevel::WARNING);
    }

    void error(const std::string &message) const override
    {
        log(act::foundation::LogsLevel::ERR, message);
    }

    [[nodiscard]] act::foundation::LoggerStream errorStream() const override
    {
        return logStream(act::foundation::LogsLevel::ERR);
    }

    void fatal(const std::string &message) const override
    {
        log(act::foundation::LogsLevel::FATAL, message);
    }

    [[nodiscard]] act::foundation::LoggerStream fatalStream() const override
    {
        return logStream(act::foundation::LogsLevel::FATAL);
    }

    [[nodiscard]] bool wouldBeLogged(act::foundation::LogsLevel::Enum /*level*/) const override
    {
        return true;
    }

    [[nodiscard]] std::shared_ptr<act::foundation::AbsLogger> createAbsSubLogger(
        const std::string & /*category*/, act::foundation::LogsLevel::Enum /*minLevel*/) override
    {
        return std::make_shared<RecordingLogger>();
    }

    [[nodiscard]] std::shared_ptr<act::foundation::AbsLogger> createAbsSubLogger(
        act::foundation::LogsLevel::Enum /*minLevel*/) override
    {
        return std::make_shared<RecordingLogger>();
    }

  public:
    /** @brief Get every message recorded so far, oldest first */
    [[nodiscard]] const std::vector<Entry> &getEntries() const
    {
        return m_entries;
    }

    /** @brief Count the messages recorded with the given level */
    [[nodiscard]] std::size_t count(act::foundation::LogsLevel::Enum level) const
    {
        return static_cast<std::size_t>(
            std::ranges::count_if(m_entries,
                                  [level](const Entry &entry) { return entry.level == level; }));
    }

    /** @brief Test if a message with the given level contains the given text */
    [[nodiscard]] bool contains(act::foundation::LogsLevel::Enum level,
                                const std::string &text) const
    {
        return std::ranges::any_of(m_entries, [&level, &text](const Entry &entry) {
            return (entry.level == level) && (entry.message.find(text) != std::string::npos);
        });
    }

  private:
    /** @brief The recorded messages; logging is const in the interface */
    mutable std::vector<Entry> m_entries;
};

} // namespace act::tests
