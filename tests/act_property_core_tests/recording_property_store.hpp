// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file recording_property_store.hpp
 * @brief In-memory property store test double which counts its calls and can be made to fail.
 */

#pragma once

#include "act_property_core/stored_value.hpp"
#include "act_property_core/stores/in_memory_property_store.hpp"

#include <string>

namespace act::property::test
{

/** @brief In-memory store counting the calls of its operations, each of which can be made to fail
 */
class RecordingPropertyStore : public InMemoryPropertyStore
{
  public:
    RecordingPropertyStore() = default;

    ~RecordingPropertyStore() override = default;

  public:
    bool init() override;

    bool set(const std::string &key, const StoredValue &value) override;

    bool erase(const std::string &key) override;

  public:
    /** @brief Make the next init calls fail, or succeed again */
    void setFailInit(bool fail)
    {
        m_failInit = fail;
    }

    /** @brief Make the next set calls fail without writing, or succeed again */
    void setFailSet(bool fail)
    {
        m_failSet = fail;
    }

    /** @brief Make the next erase calls fail without erasing, or succeed again */
    void setFailErase(bool fail)
    {
        m_failErase = fail;
    }

    [[nodiscard]] int getInitCount() const
    {
        return m_initCount;
    }

    /** @brief Count of set calls, including the failed ones */
    [[nodiscard]] int getSetCount() const
    {
        return m_setCount;
    }

    /** @brief Count of erase calls, including the failed ones */
    [[nodiscard]] int getEraseCount() const
    {
        return m_eraseCount;
    }

  private:
    bool m_failInit{false};
    bool m_failSet{false};
    bool m_failErase{false};
    int m_initCount{0};
    int m_setCount{0};
    int m_eraseCount{0};
};

inline bool RecordingPropertyStore::init()
{
    ++m_initCount;
    return !m_failInit && InMemoryPropertyStore::init();
}

inline bool RecordingPropertyStore::set(const std::string &key, const StoredValue &value)
{
    ++m_setCount;
    return !m_failSet && InMemoryPropertyStore::set(key, value);
}

inline bool RecordingPropertyStore::erase(const std::string &key)
{
    ++m_eraseCount;
    return !m_failErase && InMemoryPropertyStore::erase(key);
}

} // namespace act::property::test
