// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file store_backed_manager.hpp
 * @brief Property manager base for the tests, working on a store owned by the test, and the
 *        fixture providing that store.
 *
 * The store outlives the managers built on it, so a test can build a second manager on the same
 * store to simulate the next start of an application.
 */

#pragma once

#include "recording_property_store.hpp"

#include "act_property_core/services/abs_property_manager.hpp"
#include "act_property_core/services/abs_property_store.hpp"

#include "act_logger/services/logger_manager.hpp"

#include <gtest/gtest.h>

namespace act::property::test
{

/** @brief Manager reaching a store it does not own; derived classes declare the descriptors */
class StoreBackedManager : public AbsPropertyManager
{
  public:
    StoreBackedManager(act::logger::LoggerManager &logger, AbsPropertyStore &store)
        : AbsPropertyManager(logger),
          m_store(store)
    {
    }

    ~StoreBackedManager() override = default;

  protected:
    AbsPropertyStore &accessStore() override
    {
        return m_store;
    }

  private:
    AbsPropertyStore &m_store;
};

/** @brief Fixture owning an initialized logger manager and a recording store */
class StoreBackedManagerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // A property manager creates its sub-logger at construction, so the logger comes first
        ASSERT_TRUE(m_loggerManager.init());
    }

    [[nodiscard]] act::logger::LoggerManager &getLoggerManager()
    {
        return m_loggerManager;
    }

    [[nodiscard]] RecordingPropertyStore &getStore()
    {
        return m_store;
    }

  private:
    act::logger::LoggerManager m_loggerManager;
    RecordingPropertyStore m_store;
};

} // namespace act::property::test
