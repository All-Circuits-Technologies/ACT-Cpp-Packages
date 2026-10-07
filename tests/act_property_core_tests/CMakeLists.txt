# SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Unit tests of act_property_core. The descriptors are exercised through test managers built on a
# recording in-memory store (store_backed_manager.hpp, recording_property_store.hpp).
act_add_unit_test(act_property_core_tests
    SOURCES
    test_abs_property_manager.cpp
    test_codec.cpp
    test_default_provider.cpp
    test_enum_codec.cpp
    test_in_memory_property_store.cpp
    test_r_property.cpp
    test_rw_property.cpp
    test_scalar_codec.cpp
    test_seed_provider.cpp
    test_serialized_codec.cpp
    test_stored_scalar_traits.cpp
    test_stored_type.cpp
    test_stored_value.cpp
    LIBS
    act_property_core
)
