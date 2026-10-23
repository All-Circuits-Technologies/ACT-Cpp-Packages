// SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "utilities/scoped_utf8_locale.hpp"

#include "act_foundation/logger/abs_logger.hpp"

namespace act::archive
{
#if defined(_WIN32)

ScopedUtf8Locale::ScopedUtf8Locale(const act::foundation::AbsLogger & /*logger*/)
{
}

ScopedUtf8Locale::~ScopedUtf8Locale() = default;

bool ScopedUtf8Locale::isActive() const
{
    return true;
}

#else

// The locale is built from scratch: its categories other than the character types are the "C"
// ones, not the ones of the process. This is harmless, the libarchive calls made in its scope only
// depend on the character types
ScopedUtf8Locale::ScopedUtf8Locale(const act::foundation::AbsLogger &logger)
    : m_utf8Locale(newlocale(LC_CTYPE_MASK, "C.UTF-8", static_cast<locale_t>(nullptr)))
{
    if (m_utf8Locale == static_cast<locale_t>(nullptr))
    {
        // errno is not reported: glibc may leave it unset when this locale is missing
        logger.warningStream()
            << "act_archive: the C.UTF-8 locale is not available, the calling thread keeps its "
               "locale: the non ASCII text converted meanwhile may be rejected or altered; "
               "install the C.UTF-8 locale";
        return;
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
