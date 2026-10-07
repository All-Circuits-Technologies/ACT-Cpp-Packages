// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_system/linux/linux_system_critical_section.hpp"

#include "act_foundation/constants/def_soft.hpp"
#include "act_logger/models/abs_logger.hpp"

#include <cerrno>  // errno
#include <cstring> // std::strerror
#include <fcntl.h>
#include <sys/file.h> // flock
#include <unistd.h>   // close

namespace act::system
{

LinuxSystemCriticalSection::LinuxSystemCriticalSection(const char *slug,
                                                       const act::logger::AbsLogger &logger)
    : AbsSystemCriticalSection(slug, logger)
{
    const std::string lockFilePath = ComputeLockFilePath(slug);
    m_fd = ::open(lockFilePath.c_str(), O_CREAT | O_RDWR, LOCK_FILE_ACCESS_RIGHTS);

    if (m_fd == -1)
    {
        m_logger.errorStream() << "Failed to create/open lock file " << lockFilePath << ": "
                               << std::strerror(errno);
    }
}

LinuxSystemCriticalSection::~LinuxSystemCriticalSection()
{
    UNUSED(leave());

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

    bool locked = (flock(m_fd, LOCK_EX) == 0);
    if (!locked)
    {
        m_logger.errorStream() << "Failed to lock critical section: " << std::strerror(errno);
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

    bool unlocked = (flock(m_fd, LOCK_UN) == 0);
    if (!unlocked)
    {
        m_logger.errorStream() << "Failed to unlock critical section: " << std::strerror(errno);
        return false;
    }

    return true;
}

std::string LinuxSystemCriticalSection::ComputeLockFilePath(const char *slug)
{
    return std::string("/tmp/") + slug + ".lock";
}

} // namespace act::system
