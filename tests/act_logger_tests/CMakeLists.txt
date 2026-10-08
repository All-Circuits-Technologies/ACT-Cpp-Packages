# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Unit tests of act_logger; the external logger is replaced by a capturing test double, except for
# the console printers whose output is captured.
act_add_unit_test(act_logger_tests
    SOURCES
    test_abs_external_logger.cpp
    test_abs_logger_manager.cpp
    test_logger_helper.cpp
    test_logger_manager.cpp
    test_multi_external_logger.cpp
    test_std_console_logger.cpp
    test_sub_logger_helper.cpp
    LIBS
    act_logger
)
