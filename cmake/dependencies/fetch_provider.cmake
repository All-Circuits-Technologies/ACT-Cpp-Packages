# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# =============================================================================
# Dependency provider for the development presets
# =============================================================================
# The project declares its third-party libraries with plain `find_package(<name> REQUIRED)` calls
# and never decides by itself where they come from.
#
# The presets of CMakePresets.json load this file through CMAKE_PROJECT_TOP_LEVEL_INCLUDES. It
# registers a dependency provider which answers find_package() for the names listed below by
# fetching pinned sources, so a developer machine or a CI runner needs no extra package for them.
# A build configured without the presets does not load this file and finds the same packages on
# the system instead.
#
# Only the names handled below are intercepted; any other find_package() call runs normally
# (Threads, SQLiteCpp, ...). Fetched libraries are declared EXCLUDE_FROM_ALL so that they are only
# built when a target links them and none of their install() rules run.
#
# Each fetched project exposes the same imported target names as its installed CMake package, so
# the consuming CMakeLists are identical in both modes.
# =============================================================================

cmake_minimum_required(VERSION 3.24)

include(FetchContent)

# -----------------------------------------------------------------------------
# Per-package fetch recipes
# -----------------------------------------------------------------------------
macro(_act_provide_gtest)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    # Keep GoogleTest from overriding the parent project's compiler/linker settings.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(googletest)
    # The source tree already defines the GTest::gtest_main alias its package exports.
endmacro()

# -----------------------------------------------------------------------------
# Provider entry point
# -----------------------------------------------------------------------------
# Called by CMake for every find_package(). Setting <name>_FOUND tells CMake the request was
# fulfilled; leaving it unset lets the regular find_package() search run.
macro(_act_fetch_provider method package_name)
    if("${package_name}" STREQUAL "GTest")
        _act_provide_gtest()
        set(GTest_FOUND TRUE)
    endif()
endmacro()

cmake_language(SET_DEPENDENCY_PROVIDER _act_fetch_provider SUPPORTED_METHODS FIND_PACKAGE)
