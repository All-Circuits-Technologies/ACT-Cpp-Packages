# SPDX-FileCopyrightText: 2026 Ghislain Mangé <ghislain.mange@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Unit tests of act_num.
act_add_unit_test(act_num_tests
    SOURCES
    test_double_util.cpp
    LIBS
    act_num
    act_test_common
)
