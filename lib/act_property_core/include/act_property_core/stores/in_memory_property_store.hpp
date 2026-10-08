// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_property_core/services/abs_property_store.hpp"
#include "act_property_core/stored_value.hpp"

#include <map>
#include <optional>
#include <string>

namespace act::property
{

/**
 * @brief A volatile store backed by an in-memory map.
 *
 * Handy for tests: it is the only store that could safely cache, since it is itself the source of
 * truth in memory. Its content does not survive the process.
 */
class InMemoryPropertyStore : public AbsPropertyStore
{
  public:
    /// @brief Nothing special for default constructor
    InMemoryPropertyStore() = default;

    /// @brief Nothing special for default destructor
    ~InMemoryPropertyStore() override = default;

  public:
    /**
     * @brief Prepare the store (nothing to do for an in-memory map)
     * @return Always true
     */
    bool init() override;

    /** @copydoc AbsPropertyStore::get */
    [[nodiscard]] std::optional<StoredValue> get(const std::string &key) const override;

    /** @copydoc AbsPropertyStore::set */
    bool set(const std::string &key, const StoredValue &value) override;

    /** @copydoc AbsPropertyStore::erase */
    bool erase(const std::string &key) override;

    /** @copydoc AbsPropertyStore::clearAll */
    bool clearAll() override;

  private:
    /** @brief The stored values, keyed by property key */
    std::map<std::string, StoredValue> m_values;
};

} // namespace act::property
