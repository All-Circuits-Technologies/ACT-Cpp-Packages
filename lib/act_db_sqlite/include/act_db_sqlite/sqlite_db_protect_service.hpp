// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_db_core/services/db_protect_service.hpp"
#include "act_db_sqlite/sqlite_db_manager.hpp"

namespace act::db::sqlite
{
class SQLiteDbProtectService : public act::db::core::DbProtectService<ASqLiteDbManager>
{
  public:
    explicit SQLiteDbProtectService(ASqLiteDbManager &db, act::foundation::AbsLogger &parentLogger)
        : DbProtectService(db, parentLogger)
    {
    }
};
} // namespace act::db::sqlite
