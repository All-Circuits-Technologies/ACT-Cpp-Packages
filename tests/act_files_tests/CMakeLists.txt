# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Unit tests of act_files.
act_add_unit_test(act_files_tests
    SOURCES
    test_ext_file.cpp
    test_file_util.cpp
    LIBS
    act_files
    act_test_common
)
