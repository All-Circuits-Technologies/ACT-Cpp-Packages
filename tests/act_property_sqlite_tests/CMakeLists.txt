# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Unit tests of act_property_sqlite, run against real SQLite database files.
act_add_unit_test(act_property_sqlite_tests
    SOURCES
    test_sqlite_property_store.cpp
    LIBS
    act_property_sqlite
    act_test_common
)
