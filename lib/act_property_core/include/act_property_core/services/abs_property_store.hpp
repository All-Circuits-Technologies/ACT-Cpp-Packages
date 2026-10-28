// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/services/abs_service.hpp"
#include "act_property_core/stored_value.hpp"

#include <optional>
#include <string>

namespace act::property
{

/**
 * @brief A raw key -> typed value dictionary, with no notion of default value.
 *
 * The store is the only source of truth: there is no cache, every read goes to the store. Default
 * values and realignment are the manager's responsibility, not the store's. A store is a
 * sub-service (it derives from @ref act::foundation::AbsService): it is not registered in the
 * global manager but owned and initialized by its property manager. Its @ref init prepares the
 * backing storage.
 *
 * This package ships an in-memory implementation (@ref InMemoryPropertyStore, handy for tests); a
 * persistent store implements this interface in a package of its own.
 */
class AbsPropertyStore : public act::foundation::AbsService
{
  protected:
    /// @brief Nothing special for default constructor
    AbsPropertyStore() = default;

  public:
    /// @brief Nothing special for default destructor
    ~AbsPropertyStore() override = default;

    /**
     * @brief Prepare the store (for example, create the table when asked to).
     * @return true if the store is ready, false otherwise
     */
    bool init() override = 0;

    /**
     * @brief Read the raw value stored for a key
     * @note A read error (I/O error, locked database, ...) cannot be told apart from an absent key:
     *       it reads as absent, so a descriptor then falls back on its default, and a seed is
     *       written again at init. An implementation must log such an error, as it is the only
     *       place which can tell them apart.
     * @param key The key to read
     * @return The stored value, or an empty optional if the key is absent or cannot be read
     */
    [[nodiscard]] virtual std::optional<StoredValue> get(const std::string &key) const = 0;

    /**
     * @brief Create or replace the value stored for a key (upsert)
     * @param key The key to write
     * @param value The value to store
     * @return true on success, false otherwise
     */
    virtual bool set(const std::string &key, const StoredValue &value) = 0;

    /**
     * @brief Remove a key if it exists
     * @param key The key to remove
     * @return true on success (including when the key was already absent), false otherwise
     */
    virtual bool erase(const std::string &key) = 0;

    /**
     * @brief Remove every key from the store (for example a factory reset of this store)
     * @return true on success, false otherwise
     */
    virtual bool clearAll() = 0;
};

} // namespace act::property
