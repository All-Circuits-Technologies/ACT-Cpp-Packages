// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_csv_util.cpp
 * @brief Unit tests of CsvUtil.
 *
 * Covers escaping a value, building a CSV line, appending lines to a file (in a temporary
 * directory removed after each test), splitting CSV content into lines, and splitting a line into
 * trimmed values.
 */

#include "act_files/ext_file.hpp"
#include "act_files/file_util.hpp"
#include "act_test_common/recording_logger.hpp"
#include "act_text/csv_util.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace act::text::CsvUtil
{
namespace
{

    using ::testing::ElementsAre;
    using ::testing::IsEmpty;

    /**
     * @brief Read the fields of a CSV line as RFC 4180 describes them: the reverse of the escaping
     * of the values and of the creation of the line
     */
    std::vector<std::string> ReadRfc4180Fields(const std::string &line, char separator)
    {
        std::vector<std::string> fields;
        std::string field;
        bool inQuotes = false;

        for (std::size_t index = 0; index < line.size(); ++index)
        {
            const char character = line[index];
            if (inQuotes)
            {
                if (character != '"')
                {
                    field += character;
                }
                else if ((index + 1 < line.size()) && (line[index + 1] == '"'))
                {
                    field += '"';
                    ++index;
                }
                else
                {
                    inQuotes = false;
                }
            }
            else if (character == '"')
            {
                inQuotes = true;
            }
            else if (character == separator)
            {
                fields.push_back(field);
                field.clear();
            }
            else if (character != '\n')
            {
                field += character;
            }
        }
        fields.push_back(field);

        return fields;
    }

    TEST(CsvUtilTest, EscapeCsvValueKeepsAPlainValue)
    {
        EXPECT_EQ(EscapeCsvValue("plain value"), "plain value");
        EXPECT_EQ(EscapeCsvValue(" padded "), " padded ");
        EXPECT_EQ(EscapeCsvValue(""), "");
    }

    TEST(CsvUtilTest, EscapeCsvValueQuotesAValueHoldingTheSeparator)
    {
        EXPECT_EQ(EscapeCsvValue("a,b"), "\"a,b\"");
        EXPECT_EQ(EscapeCsvValue("a;b", ";"), "\"a;b\"");
        EXPECT_EQ(EscapeCsvValue("a||b", "||"), "\"a||b\"");
    }

    TEST(CsvUtilTest, EscapeCsvValueKeepsAValueHoldingAnotherSeparator)
    {
        EXPECT_EQ(EscapeCsvValue("a;b"), "a;b");
        EXPECT_EQ(EscapeCsvValue("a,b", ";"), "a,b");
        EXPECT_EQ(EscapeCsvValue("a|b", "||"), "a|b");
    }

    TEST(CsvUtilTest, EscapeCsvValueQuotesAValueHoldingALineBreak)
    {
        EXPECT_EQ(EscapeCsvValue("two\nlines"), "\"two\nlines\"");
        EXPECT_EQ(EscapeCsvValue("two\rlines"), "\"two\rlines\"");
        EXPECT_EQ(EscapeCsvValue("two\r\nlines", ";"), "\"two\r\nlines\"");
    }

    TEST(CsvUtilTest, EscapeCsvValueQuotesAndDoublesTheDoubleQuotes)
    {
        EXPECT_EQ(EscapeCsvValue("say \"hi\""), "\"say \"\"hi\"\"\"");
        EXPECT_EQ(EscapeCsvValue("\""), "\"\"\"\"");
        EXPECT_EQ(EscapeCsvValue("\"\"", ";"), "\"\"\"\"\"\"");
    }

    TEST(CsvUtilTest, EscapeCsvValueOfAnEmptySeparatorOnlyLooksAtQuotesAndLineBreaks)
    {
        EXPECT_EQ(EscapeCsvValue("a,b", ""), "a,b");
        EXPECT_EQ(EscapeCsvValue("a\"b", ""), "\"a\"\"b\"");
    }

    TEST(CsvUtilTest, EscapeCsvValueKeepsTheColumnsOfALine)
    {
        const std::vector<std::string> values =
            {"plain", "a,b", "say \"hi\"", "two\nlines", "cr\r\nlf", "", "\"", " padded "};
        std::vector<std::string> escapedValues;
        for (const std::string &value : values)
        {
            escapedValues.push_back(EscapeCsvValue(value));
        }

        EXPECT_EQ(ReadRfc4180Fields(CreateCsvLine(escapedValues), ','), values);
    }

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
