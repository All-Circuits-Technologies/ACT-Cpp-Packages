# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Unit tests of act_threading.
act_add_unit_test(act_threading_tests
    SOURCES
    test_mutexed.cpp
    test_reusable_thread.cpp
    LIBS
    act_threading
)
