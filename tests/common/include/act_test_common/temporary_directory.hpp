// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file temporary_directory.hpp
 * @brief Test fixture owning a temporary directory and a recording logger.
 */

#pragma once

#include "act_test_common/recording_logger.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <random>
#include <string>

namespace act::tests
{

/** @brief Gives each test an empty temporary directory, removed with everything it contains */
class TemporaryDirectoryTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        std::random_device random;
        m_directory = std::filesystem::temp_directory_path() /
                      ("act_tests_" + std::to_string(random()) + std::to_string(random()));
        ASSERT_TRUE(std::filesystem::create_directory(m_directory));
    }

    void TearDown() override
    {
        std::filesystem::remove_all(m_directory);
    }

    /** @brief Get the path of a file inside the temporary directory */
    [[nodiscard]] std::string getPath(const std::string &fileName) const
    {
        return (m_directory / fileName).string();
    }

    [[nodiscard]] const std::filesystem::path &getDirectory() const
    {
        return m_directory;
    }

    [[nodiscard]] RecordingLogger &getLogger()
    {
        return m_logger;
    }

  private:
    std::filesystem::path m_directory;
    RecordingLogger m_logger;
};

} // namespace act::tests
