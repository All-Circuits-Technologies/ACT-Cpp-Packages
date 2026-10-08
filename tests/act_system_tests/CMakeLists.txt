# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Unit tests of act_system. Only the Linux implementations are tested, and the reboot is never
# asked: the commands run are harmless shell built-ins.
act_add_unit_test(act_system_tests
    SOURCES
    test_abs_system_manager.cpp
    test_linux_system_critical_section.cpp
    test_linux_system_manager.cpp
    test_system_critical_section_guard.cpp
    LIBS
    act_system
)
