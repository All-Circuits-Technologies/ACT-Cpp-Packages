# SPDX-FileCopyrightText: 2026 Théo Magne <theo.magne@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# -----------------------------------------------------------------------------
# act_files - File system operations
# -----------------------------------------------------------------------------
# File I/O, path manipulation, and file management utilities.
# Provides file operations and extended file abstractions.
#
# Dependencies: act_foundation, act_logger
# -----------------------------------------------------------------------------

add_library(act_files STATIC
    src/ext_file.cpp
    src/file_path_util.cpp
    src/file_util.cpp
)

# Public headers are exposed via include/<module_name>/
target_include_directories(act_files
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# C++20 required
target_compile_features(act_files PUBLIC cxx_std_20)

# Dependencies
target_link_libraries(act_files
    PUBLIC
    act_foundation
    act_logger
)

# Install targets
install(TARGETS act_files DESTINATION lib)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "include"
    FILES_MATCHING PATTERN "*.hpp"
)
