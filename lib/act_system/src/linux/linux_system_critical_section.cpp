// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_system/linux/linux_system_critical_section.hpp"

#include "act_foundation/logger/abs_logger.hpp"
#include "act_system/abs_system_critical_section.hpp"

#include <cerrno> // errno
#include <fcntl.h>
#include <string>
#include <sys/file.h> // flock
#include <system_error>
#include <tuple>
#include <unistd.h> // close

namespace act::system
{

LinuxSystemCriticalSection::LinuxSystemCriticalSection(const char *slug,
                                                       const act::foundation::AbsLogger &logger)
    : AbsSystemCriticalSection(slug, logger)
{
    const std::string lockFilePath = ComputeLockFilePath(slug);
    // open is variadic in POSIX: the mode is its third argument
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    m_fd = ::open(lockFilePath.c_str(), O_CREAT | O_RDWR, LOCK_FILE_ACCESS_RIGHTS);

    if (m_fd == -1)
    {
        m_logger.errorStream() << "Failed to create/open lock file " << lockFilePath << ": "
                               << std::generic_category().message(errno);
    }
}

LinuxSystemCriticalSection::~LinuxSystemCriticalSection()
{
    // Qualified call: a destructor must not dispatch to a derived class, which is already gone
    std::ignore = LinuxSystemCriticalSection::leave();

    if (m_fd >= 0)
    {
        ::close(m_fd);
    }
}

bool LinuxSystemCriticalSection::enter() const
{
    if (m_fd < 0)
    {
        m_logger.error("Invalid lock file descriptor");
        return false;
    }

    const bool locked = (flock(m_fd, LOCK_EX) == 0);
    if (!locked)
    {
        m_logger.errorStream() << "Failed to lock critical section: "
                               << std::generic_category().message(errno);
        return false;
    }

    return true;
}

bool LinuxSystemCriticalSection::leave() const
{
    if (m_fd < 0)
    {
        m_logger.error("Invalid lock file descriptor");
        return false;
    }

    const bool unlocked = (flock(m_fd, LOCK_UN) == 0);
    if (!unlocked)
    {
        m_logger.errorStream() << "Failed to unlock critical section: "
                               << std::generic_category().message(errno);
        return false;
    }

    return true;
}

std::string LinuxSystemCriticalSection::ComputeLockFilePath(const char *slug)
{
    return std::string("/tmp/") + slug + ".lock";
}

} // namespace act::system
