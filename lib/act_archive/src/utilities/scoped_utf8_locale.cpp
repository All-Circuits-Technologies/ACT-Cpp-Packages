// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "utilities/scoped_utf8_locale.hpp"

#include "act_foundation/logger/abs_logger.hpp"
#include "act_text/string_util.hpp"

namespace act::archive
{
#ifdef _WIN32

ScopedUtf8Locale::ScopedUtf8Locale(const act::foundation::AbsLogger &logger)
    : ScopedUtf8Locale(logger, UTF8_LOCALE_NAMES)
{
}

ScopedUtf8Locale::ScopedUtf8Locale(const act::foundation::AbsLogger & /*logger*/,
                                   std::span<const char *const> /*localeNames*/)
{
}

ScopedUtf8Locale::~ScopedUtf8Locale() = default;

bool ScopedUtf8Locale::isActive() const
{
    return true;
}

#else

ScopedUtf8Locale::ScopedUtf8Locale(const act::foundation::AbsLogger &logger)
    : ScopedUtf8Locale(logger, UTF8_LOCALE_NAMES)
{
}

// Each locale is built from scratch: its categories other than the character types are the "C"
// ones, not the ones of the process. This is harmless, the libarchive calls made in its scope only
// depend on the character types
ScopedUtf8Locale::ScopedUtf8Locale(const act::foundation::AbsLogger &logger,
                                   std::span<const char *const> localeNames)
{
    /* Take the first available locale */
    const char *usedLocaleName = nullptr;
    for (const char *localeName : localeNames)
    {
        m_utf8Locale = newlocale(LC_CTYPE_MASK, localeName, static_cast<locale_t>(nullptr));
        if (m_utf8Locale != static_cast<locale_t>(nullptr))
        {
            usedLocaleName = localeName;
            break;
        }
    }

    if (!isActive())
    {
        // errno is not reported: glibc may leave it unset when such a locale is missing
        logger.warningStream() << "act_archive: no UTF-8 locale available ("
                               << act::text::StringUtil::IterableToString(localeNames, ", ")
                               << "): non ASCII names unsupported";
        return;
    }

    if (usedLocaleName != localeNames.front())
    {
        logger.debugStream() << "act_archive: the " << localeNames.front()
                             << " locale is not available, using " << usedLocaleName;
    }
    m_previousLocale = uselocale(m_utf8Locale);
}

ScopedUtf8Locale::~ScopedUtf8Locale()
{
    if (m_utf8Locale == static_cast<locale_t>(nullptr))
    {
        return;
    }

    uselocale(m_previousLocale);
    freelocale(m_utf8Locale);
}

bool ScopedUtf8Locale::isActive() const
{
    return m_utf8Locale != static_cast<locale_t>(nullptr);
}

#endif
} // namespace act::archive
