# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Unit tests of act_time. The timers run on their own threads, so these tests wait on conditions
# with generous deadlines.
act_add_unit_test(act_time_tests
    SOURCES
    test_action_delayer.cpp
    test_date_time_util.cpp
    test_restartable_timer.cpp
    LIBS
    act_time
)
