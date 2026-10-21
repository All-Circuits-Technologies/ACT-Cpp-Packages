# SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# -----------------------------------------------------------------------------
# act_archive - Archive reading and writing
# -----------------------------------------------------------------------------
# Writes and reads archives held in memory, ZIP for now, nested archives included,
# with safe entry paths and size limits.
#
# Dependencies: act_foundation, act_text
# External: libarchive (libarchive-dev)
# -----------------------------------------------------------------------------

if(NOT TARGET LibArchive::LibArchive)
    find_package(LibArchive REQUIRED)
endif()

add_library(act_archive STATIC
    src/reader/archive_reader.cpp
    src/reader/archive_reader_impl.cpp
    src/utilities/archive_path_util.cpp
    src/utilities/libarchive_util.cpp
    src/utilities/scoped_utf8_locale.cpp
    src/writer/archive_writer.cpp
    src/writer/archive_writer_impl.cpp
    src/writer/string_archive_sink.cpp
)

# Public headers are exposed via include/<module_name>/
target_include_directories(act_archive
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    # Internal headers, shared by the sources only
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# C++20 required
target_compile_features(act_archive PUBLIC cxx_std_20)

# Dependencies; libarchive stays out of the public headers
target_link_libraries(act_archive
    PUBLIC
    act_foundation
    PRIVATE
    act_text
    LibArchive::LibArchive
)

# Install targets
install(TARGETS act_archive DESTINATION lib)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "include"
    FILES_MATCHING PATTERN "*.hpp"
)
