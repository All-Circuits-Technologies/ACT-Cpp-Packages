// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_logger/services/abs_logger_manager.hpp"

#include "act_logger/helpers/logger_helper.hpp"

#include <memory>

namespace act::logger
{

AbsLoggerManager::AbsLoggerManager()

{
}

bool AbsLoggerManager::init()
{
    auto externalLogger = getExternalLogger();
    m_loggerHelper = std::make_shared<LoggerHelper>(externalLogger);

    info("LoggerManager initialized successfully.");
    return true;
}

} // namespace act::logger
