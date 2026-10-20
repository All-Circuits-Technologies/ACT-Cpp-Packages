# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Unit tests of act_db_sqlite, run against real SQLite database files in a temporary directory.
act_add_unit_test(act_db_sqlite_tests
    SOURCES
    test_sqlite_db_manager.cpp
    test_sqlite_db_protect_service.cpp
    LIBS
    act_db_sqlite
)
