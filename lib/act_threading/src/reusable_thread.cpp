// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_threading/reusable_thread.hpp"

namespace act::threading
{

ReusableThread::~ReusableThread()
{
    if (m_waitingThread != nullptr)
    {
        if (m_waitingThread->joinable())
        {
            m_waitingThread->join();
        }
        delete m_waitingThread;
        m_waitingThread = nullptr;

        // Waiting thread joined, so working thread is not running anymore
        delete m_workingThread;
        m_workingThread = nullptr;
    }
}

bool ReusableThread::isRunning() const
{
    // Only the atomic flag: start() may replace the working thread meanwhile, so reading it here
    // would race. The flag is set before the working thread is started and cleared once it ended.
    return m_threadRunning.load();
}

ReusableThreadResult::Enum ReusableThread::startProcess(
    bool waitToJoin,
    const act::foundation::AbsLogger &logger,
    const std::function<std::thread *()> &threadFactory)
{
    // This mutex protects the start process in case start() is called from different threads
    std::lock_guard<std::mutex> lock(m_startThreadMutex);

    if (m_waitingThread != nullptr && m_waitingThread->joinable())
    {
        // If there is a waiting thread, it means that the working thread is not null
        if (m_threadRunning.load())
        {
            if (!waitToJoin)
            {
                return ReusableThreadResult::Enum::ALREADY_RUNNING;
            }

            logger.debugStream() << "Waiting for previous thread: " << m_workingThread->get_id()
                                 << ", to finish...";
        }

        // Join the previous thread, either because we had to wait or because it has ended
        m_waitingThread->join();
    }

    if (m_workingThread != nullptr)
    {
        delete m_workingThread;
        m_workingThread = nullptr;
    }

    if (m_waitingThread != nullptr)
    {
        delete m_waitingThread;
        m_waitingThread = nullptr;
    }

    // Call precondition function to get the condition
    m_workingThread = threadFactory();
    if (m_workingThread == nullptr)
    {
        return ReusableThreadResult::Enum::INTERNAL_ERROR;
    }

    m_threadRunning.store(true);
    m_waitingThread = new std::thread(WaitingThreadFunction, this);

    return ReusableThreadResult::Enum::OK;
}

void ReusableThread::WaitingThreadFunction(ReusableThread *currentThread)
{
    if (currentThread->m_workingThread != nullptr && currentThread->m_workingThread->joinable())
    {
        currentThread->m_workingThread->join();
    }

    currentThread->m_threadRunning.store(false);
}

} // namespace act::threading
