# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# -----------------------------------------------------------------------------
# act_property_sqlite - SQLite backed property store
# -----------------------------------------------------------------------------
# A property store persisting the properties in a key/value table of a SQLite
# database, either an application database or a dedicated one.
#
# Dependencies: act_property_core, act_db_core, act_db_sqlite, act_logger
# External: SQLiteCpp (libsqlitecpp-dev)
# -----------------------------------------------------------------------------

if(NOT TARGET SQLiteCpp)
    find_package(SQLiteCpp REQUIRED)
endif()

add_library(act_property_sqlite STATIC
    # Stores
    src/stores/sqlite_property_store.cpp
)

target_include_directories(act_property_sqlite
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(act_property_sqlite PUBLIC cxx_std_20)

target_link_libraries(act_property_sqlite
    PUBLIC
    act_property_core
    act_db_core
    act_db_sqlite
    act_logger
    PRIVATE
    SQLiteCpp
)

install(TARGETS act_property_sqlite DESTINATION lib)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "include"
    FILES_MATCHING PATTERN "*.hpp"
)
