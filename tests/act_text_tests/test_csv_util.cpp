// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

/**
 * @file test_csv_util.cpp
 * @brief Unit tests of CsvUtil.
 *
 * Covers
 * - escaping a value (quoted when it holds the separator, a double quote or a line break, or
 *   starts or ends with whitespace),
 * - building a CSV line (with or without escaping its values),
 * - appending lines to a file (in a temporary directory removed after each test),
 * - splitting CSV content into lines (a line ends with LF or CRLF, a line break between double
 *   quotes does not end one, a UTF-8 byte order mark at the beginning is skipped),
 * - removing the blank lines,
 * - splitting a line into values (trimmed, or read as they are when they are between double
 *   quotes), and
 * - finding columns by their title (trimmed, without case, ambiguous when found several times).
 */

#include "act_files/ext_file.hpp"
#include "act_files/file_util.hpp"
#include "act_test_common/recording_logger.hpp"
#include "act_text/csv_util.hpp"
#include "act_text/string_util.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <optional> // IWYU pragma: keep - cf. std::optional
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace act::text::CsvUtil
{
namespace
{

    using ::testing::ElementsAre;
    using ::testing::ElementsAreArray;
    using ::testing::IsEmpty;

    /** @brief The UTF-8 byte order mark, as the text a CSV file may start with */
    using act::text::StringUtil::UTF8_BYTE_ORDER_MARK;

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
        EXPECT_EQ(EscapeCsvValue(""), "");
    }

    TEST(CsvUtilTest, EscapeCsvValueQuotesAValueStartingOrEndingWithWhitespace)
    {
        EXPECT_EQ(EscapeCsvValue(" padded "), "\" padded \"");
        EXPECT_EQ(EscapeCsvValue(" leading"), "\" leading\"");
        EXPECT_EQ(EscapeCsvValue("trailing\t"), "\"trailing\t\"");
        EXPECT_EQ(EscapeCsvValue(" "), "\" \"");
    }

    TEST(CsvUtilTest, ParseCsvLineReadsBackAValueStartingOrEndingWithWhitespace)
    {
        const std::vector<std::string> values = {" padded ", " leading", "trailing ", "plain"};

        EXPECT_THAT(ParseCsvLine(CreateCsvLine(values, ";"), ";"), ElementsAreArray(values));
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

        EXPECT_EQ(ReadRfc4180Fields(JoinCsvLine(escapedValues), ','), values);
    }

    TEST(CsvUtilTest, JoinCsvLineJoinsValuesAndEndsTheLine)
    {
        EXPECT_EQ(JoinCsvLine({"a", "b", "c"}), "a,b,c\n");
        EXPECT_EQ(JoinCsvLine({"a", "b"}, ";"), "a;b\n");
    }

    TEST(CsvUtilTest, JoinCsvLineOfNoValueIsAnEmptyLine)
    {
        EXPECT_EQ(JoinCsvLine({}), "\n");
    }

    TEST(CsvUtilTest, CreateCsvLineJoinsPlainValuesAsJoinCsvLineDoes)
    {
        EXPECT_EQ(CreateCsvLine({"a", "b", "c"}), "a,b,c\n");
        EXPECT_EQ(CreateCsvLine({"a", "b"}, ";"), "a;b\n");
    }

    TEST(CsvUtilTest, CreateCsvLineEscapesTheValuesForItsSeparator)
    {
        EXPECT_EQ(CreateCsvLine({"a,b", "c;d"}), "\"a,b\",c;d\n");
        EXPECT_EQ(CreateCsvLine({"a,b", "c;d"}, ";"), "a,b;\"c;d\"\n");
    }

    TEST(CsvUtilTest, CreateCsvLineKeepsTheColumnsOfALine)
    {
        const std::vector<std::string> values =
            {"plain", "a,b", "x;y", "say \"hi\"", "two\nlines", "cr\r\nlf", "", "\"", " padded "};

        EXPECT_EQ(ReadRfc4180Fields(CreateCsvLine(values), ','), values);
        EXPECT_EQ(ReadRfc4180Fields(CreateCsvLine(values, ";"), ';'), values);
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

    TEST(CsvUtilTest, ParseCsvFileEndsTheLinesWithCrlfOrLf)
    {
        EXPECT_THAT(ParseCsvFile("a,b\r\nc,d\r\n"), ElementsAre("a,b", "c,d"));
        EXPECT_THAT(ParseCsvFile("a\r\nb\nc"), ElementsAre("a", "b", "c"));
        EXPECT_THAT(ParseCsvFile("a\r\n\r\nb"), ElementsAre("a", "", "b"));
    }

    TEST(CsvUtilTest, ParseCsvFileKeepsACarriageReturnWhichIsNotAPartOfALineBreak)
    {
        EXPECT_THAT(ParseCsvFile("a\rb\nc\r"), ElementsAre("a\rb", "c\r"));
        EXPECT_THAT(ParseCsvFile("\"x\r\ny\",b\r\nc"), ElementsAre("\"x\r\ny\",b", "c"));
    }

    TEST(CsvUtilTest, ParseCsvFileKeepsEmptyLinesInTheMiddle)
    {
        EXPECT_THAT(ParseCsvFile("a\n\nb\n"), ElementsAre("a", "", "b"));
    }

    TEST(CsvUtilTest, ParseCsvFileOfEmptyContentHasNoLine)
    {
        EXPECT_THAT(ParseCsvFile(""), IsEmpty());
    }

    TEST(CsvUtilTest, ParseCsvFileKeepsTheLineBreaksOfAQuotedValueInOneLine)
    {
        EXPECT_THAT(ParseCsvFile("a,\"two\nlines\"\nc,d\n"),
                    ElementsAre("a,\"two\nlines\"", "c,d"));
        EXPECT_THAT(ParseCsvFile("\"cr\r\nlf\",b\nc"), ElementsAre("\"cr\r\nlf\",b", "c"));
        EXPECT_THAT(ParseCsvFile("a, \"x\ny\"\nb"), ElementsAre("a, \"x\ny\"", "b"));
        EXPECT_THAT(ParseCsvFile("a\n\"x\ny\"\nb"), ElementsAre("a", "\"x\ny\"", "b"));
    }

    TEST(CsvUtilTest, ParseCsvFileKeepsTheDoubledDoubleQuotesOfAQuotedValueInOneLine)
    {
        EXPECT_THAT(ParseCsvFile("\"say \"\"hi\"\"\nthere\"\nb"),
                    ElementsAre("\"say \"\"hi\"\"\nthere\"", "b"));
        EXPECT_THAT(ParseCsvFile("\"\"\nb"), ElementsAre("\"\"", "b"));
    }

    TEST(CsvUtilTest, ParseCsvFileLooksForTheQuotesAfterTheSeparatorItIsGiven)
    {
        EXPECT_THAT(ParseCsvFile("a;\"x\ny\"\nb;c", ";"), ElementsAre("a;\"x\ny\"", "b;c"));
        EXPECT_THAT(ParseCsvFile("a;\"x\ny\"\nb;c"), ElementsAre("a;\"x", "y\"", "b;c"));
    }

    TEST(CsvUtilTest, ParseCsvFileKeepsADoubleQuoteWhichDoesNotStartAValue)
    {
        EXPECT_THAT(ParseCsvFile("5\" pipe,b\nc,d"), ElementsAre("5\" pipe,b", "c,d"));
    }

    TEST(CsvUtilTest, ParseCsvFileTakesTheRestOfTheContentWhenAQuoteIsNotClosed)
    {
        EXPECT_THAT(ParseCsvFile("a,\"b\nc\nd"), ElementsAre("a,\"b\nc\nd"));
    }

    TEST(CsvUtilTest, ParseCsvFileReversesCreateCsvLine)
    {
        const std::vector<std::string> first = {"plain", "two\nlines", "a;b", "say \"hi\""};
        const std::vector<std::string> second = {"", "cr\r\nlf", "\"", "last"};

        const std::vector<std::string> lines =
            ParseCsvFile(CreateCsvLine(first, ";") + CreateCsvLine(second, ";"), ";");

        ASSERT_EQ(lines.size(), 2U);
        EXPECT_THAT(ParseCsvLine(lines[0], ";"), ElementsAreArray(first));
        EXPECT_THAT(ParseCsvLine(lines[1], ";"), ElementsAreArray(second));
    }

    TEST(CsvUtilTest, ParseCsvFileSkipsTheByteOrderMarkAtTheBeginning)
    {
        const std::string byteOrderMark{UTF8_BYTE_ORDER_MARK};

        EXPECT_THAT(ParseCsvFile(byteOrderMark + "a,b\nc,d\n"), ElementsAre("a,b", "c,d"));
        /* The quotes of a first value are read as such: its line break stays in its line */
        EXPECT_THAT(ParseCsvFile(byteOrderMark + "\"two\nlines\",b\nc,d\n"),
                    ElementsAre("\"two\nlines\",b", "c,d"));
        EXPECT_THAT(ParseCsvFile(byteOrderMark), IsEmpty());
    }

    TEST(CsvUtilTest, ParseCsvFileKeepsWhatIsNotTheByteOrderMarkAtTheBeginning)
    {
        const std::string byteOrderMark{UTF8_BYTE_ORDER_MARK};
        const std::string lookAlike{"\xEF\xBB\xBE"};

        EXPECT_THAT(ParseCsvFile("a\n" + byteOrderMark + "b\n"),
                    ElementsAre("a", byteOrderMark + "b"));
        EXPECT_THAT(ParseCsvFile(byteOrderMark + byteOrderMark + "a\n"),
                    ElementsAre(byteOrderMark + "a"));
        EXPECT_THAT(ParseCsvFile(lookAlike + "a,b\n"), ElementsAre(lookAlike + "a,b"));
    }

    TEST(CsvUtilTest, RemoveBlankLinesRemovesTheLinesWithoutAnyValue)
    {
        std::vector<std::string> lines = {"a;b", "", "  ", ";;;", " ; \t", "\"\";", "c;d", "e"};

        RemoveBlankLines(lines, ";");

        EXPECT_THAT(lines, ElementsAre("a;b", "c;d", "e"));
    }

    TEST(CsvUtilTest, RemoveBlankLinesUsesTheSeparatorItIsGiven)
    {
        std::vector<std::string> lines = {",,", ";;"};

        RemoveBlankLines(lines);

        EXPECT_THAT(lines, ElementsAre(";;"));
    }

    TEST(CsvUtilTest, RemoveBlankLinesOfNoLineHasNoLine)
    {
        std::vector<std::string> lines;

        RemoveBlankLines(lines);

        EXPECT_THAT(lines, IsEmpty());
    }

    TEST(CsvUtilTest, ParseCsvLineSplitsAndTrimsValues)
    {
        EXPECT_THAT(ParseCsvLine(" a , b,c "), ElementsAre("a", "b", "c"));
        EXPECT_THAT(ParseCsvLine("a;\tb ;c", ";"), ElementsAre("a", "b", "c"));
    }

    TEST(CsvUtilTest, ParseCsvLineKeepsEmptyValues)
    {
        EXPECT_THAT(ParseCsvLine("a,,  ,b"), ElementsAre("a", "", "", "b"));
        EXPECT_THAT(ParseCsvLine("a,"), ElementsAre("a", ""));
        EXPECT_THAT(ParseCsvLine("\"a\",\"b\","), ElementsAre("a", "b", ""));
    }

    TEST(CsvUtilTest, ParseCsvLineOfEmptyLineHasNoValue)
    {
        EXPECT_THAT(ParseCsvLine(""), IsEmpty());
    }

    TEST(CsvUtilTest, ParseCsvLineReversesJoinCsvLine)
    {
        std::string line = JoinCsvLine({"first", "second", "third"});

        EXPECT_THAT(ParseCsvLine(line), ElementsAre("first", "second", "third"));
    }

    TEST(CsvUtilTest, ParseCsvLineReadsAValueBetweenDoubleQuotes)
    {
        EXPECT_THAT(ParseCsvLine("a,\"b\",c"), ElementsAre("a", "b", "c"));
        EXPECT_THAT(ParseCsvLine("\"a\";\"b\"", ";"), ElementsAre("a", "b"));
        EXPECT_THAT(ParseCsvLine("\"\",b,\"\""), ElementsAre("", "b", ""));
    }

    TEST(CsvUtilTest, ParseCsvLineKeepsTheSeparatorOfAQuotedValue)
    {
        EXPECT_THAT(ParseCsvLine("\"a,b\",c"), ElementsAre("a,b", "c"));
        EXPECT_THAT(ParseCsvLine("x;\"a;b;\";y", ";"), ElementsAre("x", "a;b;", "y"));
        EXPECT_THAT(ParseCsvLine("\"a,b\";c", ";"), ElementsAre("a,b", "c"));
    }

    TEST(CsvUtilTest, ParseCsvLineUndoublesTheDoubleQuotesOfAQuotedValue)
    {
        EXPECT_THAT(ParseCsvLine("\"say \"\"hi\"\"\",b"), ElementsAre("say \"hi\"", "b"));
        EXPECT_THAT(ParseCsvLine("\"\"\"\""), ElementsAre("\""));
    }

    TEST(CsvUtilTest, ParseCsvLineKeepsTheSpacesOfAQuotedValueAndSkipsTheOnesAroundIt)
    {
        EXPECT_THAT(ParseCsvLine(" \" a \" ,b"), ElementsAre(" a ", "b"));
        EXPECT_THAT(ParseCsvLine("a,\"b\"\r\n"), ElementsAre("a", "b"));
    }

    TEST(CsvUtilTest, ParseCsvLineKeepsADoubleQuoteWhichDoesNotStartAValue)
    {
        EXPECT_THAT(ParseCsvLine("5\" pipe,b"), ElementsAre("5\" pipe", "b"));
    }

    TEST(CsvUtilTest, ParseCsvLineTakesTheRestOfALineWhenAQuoteIsNotClosed)
    {
        EXPECT_THAT(ParseCsvLine("a,\"b,c"), ElementsAre("a", "b,c"));
    }

    TEST(CsvUtilTest, ParseCsvLineKeepsWhatFollowsTheClosingQuote)
    {
        EXPECT_THAT(ParseCsvLine("\"ab\"cd,e"), ElementsAre("abcd", "e"));
    }

    TEST(CsvUtilTest, ParseCsvLineKeepsEmptyValuesOfAWhitespaceSeparator)
    {
        EXPECT_THAT(ParseCsvLine("a\t\tb", "\t"), ElementsAre("a", "", "b"));
        EXPECT_THAT(ParseCsvLine("a\t\"b\tc\"", "\t"), ElementsAre("a", "b\tc"));
    }

    TEST(CsvUtilTest, ParseCsvLineOfAnEmptySeparatorIsOneValue)
    {
        EXPECT_THAT(ParseCsvLine(" a,b ", ""), ElementsAre("a,b"));
        EXPECT_THAT(ParseCsvLine("\"a,b\"", ""), ElementsAre("a,b"));
    }

    TEST(CsvUtilTest, ParseCsvLineReversesCreateCsvLine)
    {
        /* A value which has leading or trailing spaces is not quoted by EscapeCsvValue: it is
         * trimmed */
        const std::vector<std::string> values =
            {"plain", "a,b", "a;b", "say \"hi\"", "two\nlines", "cr\r\nlf", "", "\"", "\"\""};

        for (const std::string separator : {",", ";", "\t"})
        {
            EXPECT_THAT(ParseCsvLine(CreateCsvLine(values, separator), separator),
                        ElementsAreArray(values))
                << "separator: " << separator;
        }
    }

    /** @brief Gives each test a temporary directory for the CSV files it writes */
    TEST(CsvUtilTest, FindColumnsFindsTheTitlesInAnyOrder)
    {
        EXPECT_THAT(FindColumns({"b", "a", "c"}, {"a", "c", "b"}),
                    ElementsAre(std::optional<std::size_t>{1}, 2, 0));
    }

    TEST(CsvUtilTest, FindColumnsComparesTheTitlesTrimmedAndWithoutCase)
    {
        EXPECT_THAT(FindColumns({" Profile NAME ", "x"}, {"profile name", " X"}),
                    ElementsAre(std::optional<std::size_t>{0}, 1));
    }

    TEST(CsvUtilTest, FindColumnsDoesNotFindAMissingOrAnAmbiguousTitle)
    {
        EXPECT_THAT(FindColumns({"a", "b", "A"}, {"a", "z", "b"}),
                    ElementsAre(std::nullopt, std::nullopt, std::optional<std::size_t>{1}));
        EXPECT_THAT(FindColumns({}, {"a"}), ElementsAre(std::nullopt));
        EXPECT_THAT(FindColumns({"a"}, {}), IsEmpty());
    }

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
