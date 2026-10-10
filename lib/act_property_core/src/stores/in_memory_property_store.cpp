// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_property_core/stores/in_memory_property_store.hpp"

namespace act::property
{

bool InMemoryPropertyStore::init()
{
    return true;
}

std::optional<StoredValue> InMemoryPropertyStore::get(const std::string &key) const
{
    const auto it = m_values.find(key);
    if (it == m_values.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

bool InMemoryPropertyStore::set(const std::string &key, const StoredValue &value)
{
    m_values.insert_or_assign(key, value);
    return true;
}

bool InMemoryPropertyStore::erase(const std::string &key)
{
    m_values.erase(key);
    return true;
}

bool InMemoryPropertyStore::clearAll()
{
    m_values.clear();
    return true;
}

} // namespace act::property
