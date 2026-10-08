// SPDX-FileCopyrightText: 2025 Anthony Loiseau <anthony.loiseau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_system/win/win_system_critical_section.hpp"

#include "act_foundation/logger/abs_logger.hpp"

namespace act::system
{

WinSystemCriticalSection::WinSystemCriticalSection(const char *slug,
                                                   const act::foundation::AbsLogger &logger)
    : AbsSystemCriticalSection(slug, logger)
{
    const std::string mutexName = std::string("Global\\") + slug;
    m_handle = CreateMutexA(nullptr, FALSE, mutexName.c_str());

    if (m_handle == nullptr)
    {
        m_logger.errorStream() << "Failed to create mutex '" << mutexName << "': error "
                               << GetLastError();
    }
}

WinSystemCriticalSection::~WinSystemCriticalSection()
{
    if (m_handle != nullptr)
    {
        CloseHandle(m_handle);
    }
}

bool WinSystemCriticalSection::enter() const
{
    if (m_handle == nullptr)
    {
        m_logger.error("Invalid mutex handle");
        return false;
    }

    DWORD result = WaitForSingleObject(m_handle, INFINITE);
    if (result != WAIT_OBJECT_0)
    {
        m_logger.errorStream() << "Failed to lock critical section: error " << GetLastError();
        return false;
    }

    return true;
}

bool WinSystemCriticalSection::leave() const
{
    if (m_handle == nullptr)
    {
        m_logger.error("Invalid mutex handle");
        return false;
    }

    if (!ReleaseMutex(m_handle))
    {
        m_logger.errorStream() << "Failed to unlock critical section: error " << GetLastError();
        return false;
    }

    return true;
}

} // namespace act::system
