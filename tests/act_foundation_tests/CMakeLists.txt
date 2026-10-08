# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Unit tests of act_foundation.
act_add_unit_test(act_foundation_tests
    SOURCES
    test_abs_with_life_cycle.cpp
    test_def_soft.cpp
    test_finally.cpp
    test_logger_stream.cpp
    test_logs_level.cpp
    test_not_copiable_not_movable.cpp
    test_singleton.cpp
    LIBS
    act_foundation
    act_test_common
)
