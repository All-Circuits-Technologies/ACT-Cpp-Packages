// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/not_copiable_not_movable.hpp"

#include <array>
#include <span>

#ifndef _WIN32
#include <locale.h>
#endif

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

namespace act::archive
{
/**
 * @brief Switch the calling thread to a UTF-8 character type locale for its lifetime
 * @details libarchive converts the entry names between their encoding in the archive and the
 *          current locale. Under the "C" locale a process gets when it never calls setlocale(),
 *          any non ASCII name is lost. This scope makes libarchive work in UTF-8 on the calling
 *          thread only: the locale of the process and of the other threads is left untouched.
 * @note On Windows, it does nothing: libarchive keeps the names in UTF-16 there, and the UTF-8
 *       functions act_archive calls do not depend on the locale. Not tested on Windows yet.
 * @note Internal to act_archive.
 */
class ScopedUtf8Locale : private act::foundation::NotCopiableNotMovable
{
  public:
    /**
     * @brief Switch the calling thread to the first available locale of UTF8_LOCALE_NAMES, for
     *        the character types
     * @param logger The logger to tell which locale is used, or to warn with when none is
     *               available; libarchive then works in the current locale
     */
    explicit ScopedUtf8Locale(const act::foundation::AbsLogger &logger);

    /**
     * @brief Same, trying the given locales in order: the other constructor gives
     *        UTF8_LOCALE_NAMES, the tests give others
     * @param logger The logger, see above
     * @param localeNames The names of the UTF-8 locales to try, in order
     */
    ScopedUtf8Locale(const act::foundation::AbsLogger &logger,
                     std::span<const char *const> localeNames);

    /** @brief Restore the previous locale of the calling thread */
    ~ScopedUtf8Locale() override;

  public:
    /**
     * @brief Tell whether the calling thread is switched to a UTF-8 locale
     * @return True if it is (always on Windows), false if no UTF-8 locale is available
     */
    [[nodiscard]] bool isActive() const;

  public:
    /**
     * @brief The UTF-8 locales to switch to, in order: C.UTF-8, standard but not installed
     *        everywhere, then en_US.UTF-8, often installed when C.UTF-8 is missing
     * @note Only their character types are used, which are the same: any of them will do.
     */
    static constexpr std::array<const char *, 2> UTF8_LOCALE_NAMES{"C.UTF-8", "en_US.UTF-8"};

#ifndef _WIN32
  private:
    /** @brief The UTF-8 locale, or 0 if it is not available */
    locale_t m_utf8Locale{};

    /** @brief The locale of the thread before the switch */
    locale_t m_previousLocale{};
#endif
};
} // namespace act::archive
