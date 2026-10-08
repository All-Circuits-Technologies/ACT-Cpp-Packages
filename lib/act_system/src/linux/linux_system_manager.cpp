// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_system/linux/linux_system_manager.hpp"

#include <sys/wait.h> // WIFEXITED, WEXITSTATUS
#include <unistd.h>   // sync

namespace act::system
{

FILE *LinuxSystemManager::openPipe(const std::string &cmd)
{
    return popen(cmd.c_str(), "r");
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
