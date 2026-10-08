# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# -----------------------------------------------------------------------------
# act_property_core - Engine-agnostic persistent configuration store
# -----------------------------------------------------------------------------
# Typed property descriptors, codecs, value providers, the store interface and
# the property manager base. Storage-engine agnostic; a concrete store (e.g.
# a SQLite backed one) lives in a companion package.
#
# Dependencies: act_foundation, act_logger
# -----------------------------------------------------------------------------

add_library(act_property_core STATIC
    src/stored_value.cpp
    # Services
    src/services/abs_property_manager.cpp
    # Properties
    src/properties/abs_property_base.cpp
    # Stores
    src/stores/in_memory_property_store.cpp
)

target_include_directories(act_property_core
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(act_property_core PUBLIC cxx_std_20)

target_link_libraries(act_property_core
    PUBLIC
    act_foundation
    act_logger
)

install(TARGETS act_property_core DESTINATION lib)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "include"
    FILES_MATCHING PATTERN "*.hpp"
)
