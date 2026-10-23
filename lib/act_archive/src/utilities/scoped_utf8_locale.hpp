// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/not_copiable_not_movable.hpp"

#if !defined(_WIN32)
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
     * @brief Switch the calling thread to the C.UTF-8 locale for the character types
     * @param logger The logger to warn with when the locale is not available; libarchive then
     *               works in the current locale
     */
    explicit ScopedUtf8Locale(const act::foundation::AbsLogger &logger);

    /** @brief Restore the previous locale of the calling thread */
    ~ScopedUtf8Locale() override;

  public:
    /**
     * @brief Tell whether the calling thread is switched to a UTF-8 locale
     * @return True if it is (always on Windows), false if no UTF-8 locale is available
     */
    [[nodiscard]] bool isActive() const;

#if !defined(_WIN32)
  private:
    /** @brief The UTF-8 locale, or 0 if it is not available */
    locale_t m_utf8Locale{};

    /** @brief The locale of the thread before the switch */
    locale_t m_previousLocale{};
#endif
};
} // namespace act::archive
