// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_csv_util.cpp
 * @brief Unit tests of CsvUtil.
 *
 * Covers building a CSV line, appending lines to a file (in a temporary directory removed after
 * each test), splitting CSV content into lines, and splitting a line into trimmed values.
 */

#include "act_files/ext_file.hpp"
#include "act_files/file_util.hpp"
#include "act_test_common/recording_logger.hpp"
#include "act_text/csv_util.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <random>
#include <string>

namespace act::text::CsvUtil
{
namespace
{

    using ::testing::ElementsAre;
    using ::testing::IsEmpty;

    TEST(CsvUtilTest, CreateCsvLineJoinsValuesAndEndsTheLine)
    {
        EXPECT_EQ(CreateCsvLine({"a", "b", "c"}), "a,b,c\n");
        EXPECT_EQ(CreateCsvLine({"a", "b"}, ";"), "a;b\n");
    }

    TEST(CsvUtilTest, CreateCsvLineOfNoValueIsAnEmptyLine)
    {
        EXPECT_EQ(CreateCsvLine({}), "\n");
    }

    TEST(CsvUtilTest, ParseCsvFileSplitsContentIntoLines)
    {
        EXPECT_THAT(ParseCsvFile("a,b\nc,d\n"), ElementsAre("a,b", "c,d"));
        EXPECT_THAT(ParseCsvFile("a,b\nc,d"), ElementsAre("a,b", "c,d"));
    }

    TEST(CsvUtilTest, ParseCsvFileKeepsEmptyLinesInTheMiddle)
    {
        EXPECT_THAT(ParseCsvFile("a\n\nb\n"), ElementsAre("a", "", "b"));
    }

    TEST(CsvUtilTest, ParseCsvFileOfEmptyContentHasNoLine)
    {
        EXPECT_THAT(ParseCsvFile(""), IsEmpty());
    }

    TEST(CsvUtilTest, ParseCsvLineSplitsAndTrimsValues)
    {
        EXPECT_THAT(ParseCsvLine(" a , b,c "), ElementsAre("a", "b", "c"));
        EXPECT_THAT(ParseCsvLine("a;\tb ;c", ";"), ElementsAre("a", "b", "c"));
    }

    TEST(CsvUtilTest, ParseCsvLineKeepsEmptyValues)
    {
        EXPECT_THAT(ParseCsvLine("a,,  ,b"), ElementsAre("a", "", "", "b"));
    }

    TEST(CsvUtilTest, ParseCsvLineOfEmptyLineHasNoValue)
    {
        EXPECT_THAT(ParseCsvLine(""), IsEmpty());
    }

    TEST(CsvUtilTest, ParseCsvLineReversesCreateCsvLine)
    {
        std::string line = CreateCsvLine({"first", "second", "third"});

        EXPECT_THAT(ParseCsvLine(line), ElementsAre("first", "second", "third"));
    }

    /** @brief Gives each test a temporary directory for the CSV files it writes */
    class CsvUtilFileTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            std::random_device random;
            m_directory = std::filesystem::temp_directory_path() /
                          ("act_text_tests_" + std::to_string(random()) + std::to_string(random()));
            ASSERT_TRUE(std::filesystem::create_directory(m_directory));
        }

        void TearDown() override
        {
            std::filesystem::remove_all(m_directory);
        }

        [[nodiscard]] std::string getPath(const std::string &fileName) const
        {
            return (m_directory / fileName).string();
        }

        [[nodiscard]] const act::tests::RecordingLogger &getLogger() const
        {
            return m_logger;
        }

      private:
        std::filesystem::path m_directory;
        act::tests::RecordingLogger m_logger;
    };

    TEST_F(CsvUtilFileTest, AddCsvLineAppendsLinesToTheFile)
    {
        const std::string path = getPath("lines.csv");
        {
            act::files::ExtFile file(path, getLogger());
            ASSERT_TRUE(file.open(std::ios::out));

            EXPECT_TRUE(AddCsvLine({"a", "b"}, getLogger(), file));
            EXPECT_TRUE(AddCsvLine({"c", "d"}, getLogger(), file, ";"));
        }

        EXPECT_EQ(act::files::FileUtil::ReadFile(path, getLogger()),
                  std::optional<std::string>("a,b\nc;d\n"));
    }

    TEST_F(CsvUtilFileTest, AddCsvLineFailsWhenTheFileIsNotOpen)
    {
        const std::string path = getPath("closed.csv");
        act::files::ExtFile file(path, getLogger());

        EXPECT_FALSE(AddCsvLine({"a", "b"}, getLogger(), file));

        EXPECT_TRUE(getLogger().contains(act::foundation::LogsLevel::WARNING, path));
        EXPECT_FALSE(std::filesystem::exists(path));
    }

} // namespace
} // namespace act::text::CsvUtil
