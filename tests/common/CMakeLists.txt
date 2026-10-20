# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# -----------------------------------------------------------------------------
# act_test_common - Test doubles and fixtures shared by the unit tests
# -----------------------------------------------------------------------------
# Header-only, included as "act_test_common/<header>.hpp".
#
# Dependencies: act_foundation, GoogleTest
# -----------------------------------------------------------------------------

add_library(act_test_common INTERFACE)

target_include_directories(act_test_common
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(act_test_common
    INTERFACE
    act_foundation
    GTest::gtest
)
