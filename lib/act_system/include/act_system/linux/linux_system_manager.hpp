// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_system/abs_system_manager.hpp"

namespace act::system
{

/**
 * @brief Linux (POSIX) implementation of the system manager.
 *
 * Provides pipe handling via popen/pclose and calls sync() before rebooting.
 */
class LinuxSystemManager : public AbsSystemManager
{
  public:
    using AbsSystemManager::AbsSystemManager;

  protected:
    /**
     * @brief Open a pipe using the POSIX popen() call
     * @param cmd The shell command to execute
     * @return Pointer to the opened pipe, or nullptr on failure
     */
    FILE *openPipe(const std::string &cmd) override;

    /**
     * @brief Close a pipe using the POSIX pclose() call
     * @param pipe The pipe to close
     * @return The exit code of the underlying process, or -1 if the pipe cannot be closed or the
     *         process was killed by a signal
     */
    int closePipe(FILE *pipe) override;

    /**
     * @brief Call sync() to flush pending writes to disk before rebooting
     */
    void syncBeforeReboot() override;
};

} // namespace act::system
