// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_system/linux/linux_system_manager.hpp"

// popen, pclose and the wait status macros are POSIX: include-cleaner maps them to the C headers
// of the platform rather than to <cstdio> and <sys/wait.h>
// NOLINTBEGIN(misc-include-cleaner)
#include <cstdio>
#include <string>
#include <sys/wait.h> // WIFEXITED, WEXITSTATUS
#include <unistd.h>   // sync

namespace act::system
{

FILE *LinuxSystemManager::openPipe(const std::string &cmd)
{
    // Running a shell command is the purpose of this function
    return popen(cmd.c_str(), "r"); // NOLINT(cert-env33-c)
}

int LinuxSystemManager::closePipe(FILE *pipe)
{
    // pclose gives the wait status of the shell, which holds the exit code among other things
    const int status = pclose(pipe);
    if (status == -1 || !WIFEXITED(status))
    {
        // The pipe could not be closed, or the command was killed by a signal: no exit code
        return -1;
    }

    return WEXITSTATUS(status);
}

void LinuxSystemManager::syncBeforeReboot()
{
    sync();
}

} // namespace act::system

// NOLINTEND(misc-include-cleaner)
