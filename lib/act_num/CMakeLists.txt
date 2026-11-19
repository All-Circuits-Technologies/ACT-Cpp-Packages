# SPDX-FileCopyrightText: 2026 Ghislain Mangé <ghislain.mange@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# -----------------------------------------------------------------------------
# act_num - Numeric utilities
# -----------------------------------------------------------------------------
# Numeric utilities for double conversions.
# Provides DoubleUtil.
#
# Dependencies: none
# -----------------------------------------------------------------------------

add_library(act_num STATIC
)

# Public headers are exposed via include/<module_name>/
target_include_directories(act_num
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# C++20 required
target_compile_features(act_num PUBLIC cxx_std_20)

# Install targets
install(TARGETS act_num DESTINATION lib)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "include"
    FILES_MATCHING PATTERN "*.hpp"
)
