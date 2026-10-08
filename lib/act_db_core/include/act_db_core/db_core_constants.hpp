// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <string_view>

/// @brief Set of constants related to SQL transaction statements
namespace act::db::core::DbCoreConstants
{
/**
 * @brief The SQL TRANSACTION statement name
 */
constexpr std::string_view TRANSACTION_NAME = "TRANSACTION";

/**
 * @brief The SQL BEGIN statement name
 */
constexpr std::string_view BEGIN_NAME = "BEGIN";

/**
 * @brief The SQL COMMIT statement name
 */
constexpr std::string_view COMMIT_NAME = "COMMIT";

/**
 * @brief The SQL ROLLBACK statement name
 */
constexpr std::string_view ROLLBACK_NAME = "ROLLBACK";
} // namespace act::db::core::DbCoreConstants
