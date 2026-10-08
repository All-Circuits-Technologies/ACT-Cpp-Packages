# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Unit tests of act_db_core, driven through an in-memory database manager test double.
act_add_unit_test(act_db_core_tests
    SOURCES
    test_abs_db_manager.cpp
    test_db_protect_service.cpp
    test_db_transaction.cpp
    LIBS
    act_db_core
)
