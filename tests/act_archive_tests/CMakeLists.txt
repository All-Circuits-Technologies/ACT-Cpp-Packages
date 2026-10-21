# SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Unit tests of act_archive.

act_add_unit_test(act_archive_tests
    SOURCES
    test_archive_path_util.cpp
    LIBS
    act_archive
    act_test_common
)
