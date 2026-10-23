# SPDX-FileCopyrightText: 2026 Anthony Loiseau <anthony.loiseau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Unit tests of act_archive. They use libarchive directly too, to craft the archives the library
# refuses to write (see raw_archive_builder.hpp).
if(NOT TARGET LibArchive::LibArchive)
    find_package(LibArchive REQUIRED)
endif()

act_add_unit_test(act_archive_tests
    SOURCES
    test_archive_path_util.cpp
    test_archive_reader.cpp
    test_archive_writer.cpp
    LIBS
    act_archive
    act_test_common
    LibArchive::LibArchive
)
