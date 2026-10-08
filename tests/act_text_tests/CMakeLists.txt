# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Unit tests of act_text.
act_add_unit_test(act_text_tests
    SOURCES
    test_csv_util.cpp
    test_string_util.cpp
    test_vector_string_util.cpp
    LIBS
    act_text
    act_test_common
)
