# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# =============================================================================
# Unit tests
# =============================================================================
# GoogleTest-based unit tests of the libraries, discovered by CTest. Each library has its own
# directory, `<module>_tests/`, which builds one test executable from its `test_*.cpp` files.
#
# Build and run them with the `dev` presets:
#
#   cmake --preset dev && cmake --build --preset dev && ctest --preset dev
# =============================================================================

# Resolved through the fetch dependency provider with the presets, on the system otherwise (see
# cmake/dependencies/fetch_provider.cmake).
find_package(GTest REQUIRED)

include(GoogleTest)

# Discover tests at `ctest` time rather than at build time (the default POST_BUILD mode runs the
# freshly-built test binary to enumerate cases, which cannot work when cross-compiling). PRE_TEST
# keeps the build host-agnostic and defers enumeration to test time.
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

# -----------------------------------------------------------------------------
# Helper: declare a unit test executable and register it with CTest.
#
#   act_add_unit_test(<name> SOURCES <src>... LIBS <lib>...)
#
# Every test links GTest::gmock_main, so no test needs its own main() and GoogleMock is available.
# Its test cases are named `<name>.<Suite>.<Test>` in CTest.
# -----------------------------------------------------------------------------
function(act_add_unit_test test_name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})

    add_executable(${test_name} ${ARG_SOURCES})
    target_link_libraries(${test_name}
        PRIVATE
        GTest::gmock_main
        ${ARG_LIBS}
    )
    # The prefix groups the test cases by executable, so `ctest -R <module>` selects one library
    gtest_discover_tests(${test_name} TEST_PREFIX "${test_name}.")
endfunction()

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------
# Modules are listed alphabetically for simplicity.
# -----------------------------------------------------------------------------
add_subdirectory(common)

add_subdirectory(act_db_core_tests)
add_subdirectory(act_db_sqlite_tests)
add_subdirectory(act_files_tests)
