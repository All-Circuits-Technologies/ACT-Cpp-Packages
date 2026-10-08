// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/logger/logs_level.hpp"

namespace act::logger::LogsConstants
{
/** @brief Default console logs level */
constexpr const act::foundation::LogsLevel::Enum DEFAULT_CONSOLE_LOGS_LEVEL =
    act::foundation::LogsLevel::Enum::INFO;
} // namespace act::logger::LogsConstants
