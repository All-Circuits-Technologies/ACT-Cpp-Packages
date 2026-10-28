// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_scoped_utf8_locale.cpp
 * @brief Unit tests of ScopedUtf8Locale, internal to act_archive.
 *
 * Covers the switch of the calling thread to the first available UTF-8 locale and its restoring,
 * the fallback to a next locale, and the warning when none is available. A missing locale is
 * simulated with a name no system has, C.UTF-8 being installed where the tests run.
 */

#include "act_test_common/recording_logger.hpp"
#include "utilities/scoped_utf8_locale.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string>

#if !defined(_WIN32)
#include <langinfo.h>
#include <locale.h>
#endif

namespace act::archive
{
namespace
{

#if !defined(_WIN32)

    using act::foundation::LogsLevel;
    using act::tests::RecordingLogger;

    /** @brief A locale name no system has */
    constexpr const char *MISSING_LOCALE = "xx_XX.UTF-8";

    /** @brief True if the calling thread uses the global locale, as when no locale is switched */
    bool IsThreadOnTheGlobalLocale()
    {
        return uselocale(static_cast<locale_t>(nullptr)) == LC_GLOBAL_LOCALE;
    }

    /** @brief The character set of the character types of the calling thread */
    std::string ThreadCharset()
    {
        return nl_langinfo(CODESET);
    }

    TEST(ScopedUtf8LocaleTest, SwitchesToCUtf8ThenRestores)
    {
        const RecordingLogger logger;
        ASSERT_TRUE(IsThreadOnTheGlobalLocale());
        {
            const ScopedUtf8Locale utf8Locale(logger);

            EXPECT_TRUE(utf8Locale.isActive());
            EXPECT_EQ(ThreadCharset(), "UTF-8");
        }
        EXPECT_TRUE(IsThreadOnTheGlobalLocale());
        EXPECT_TRUE(logger.getEntries().empty());
    }

    TEST(ScopedUtf8LocaleTest, FallsBackToTheNextAvailableLocale)
    {
        const RecordingLogger logger;
        const std::array<const char *, 2> localeNames{MISSING_LOCALE, "C.UTF-8"};
        {
            const ScopedUtf8Locale utf8Locale(logger, localeNames);

            EXPECT_TRUE(utf8Locale.isActive());
            EXPECT_EQ(ThreadCharset(), "UTF-8");
        }
        EXPECT_TRUE(IsThreadOnTheGlobalLocale());
        EXPECT_TRUE(logger.contains(LogsLevel::DEBG,
                                    "the xx_XX.UTF-8 locale is not available, using C.UTF-8"));
        EXPECT_FALSE(logger.contains(LogsLevel::WARNING, "locale"));
    }

    TEST(ScopedUtf8LocaleTest, WarnsAndKeepsTheLocaleWhenNoneIsAvailable)
    {
        const RecordingLogger logger;
        const std::array<const char *, 2> localeNames{MISSING_LOCALE, "yy_YY.UTF-8"};
        {
            const ScopedUtf8Locale utf8Locale(logger, localeNames);

            EXPECT_FALSE(utf8Locale.isActive());
            EXPECT_TRUE(IsThreadOnTheGlobalLocale());
        }
        EXPECT_TRUE(IsThreadOnTheGlobalLocale());
        EXPECT_TRUE(logger.contains(LogsLevel::WARNING,
                                    "no UTF-8 locale available (xx_XX.UTF-8, yy_YY.UTF-8)"));
    }

#endif

} // namespace
} // namespace act::archive
