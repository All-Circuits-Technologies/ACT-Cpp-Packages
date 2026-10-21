// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_text/csv_util.hpp"

#include "act_files/ext_file.hpp"
#include "act_foundation/logger/abs_logger.hpp"
#include "act_text/string_util.hpp"
#include "act_text/vector_string_util.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>

namespace
{

/**
 * @brief Tell if a separator is found at a position of a text
 * @note An empty separator is never found: it would match at every position, while there is no
 * separator to look for.
 * @param text The text to look into
 * @param position The position to look at, which must be inside the text
 * @param separator The separator to look for
 * @return True if the separator is not empty and the text holds it at this position
 */
bool IsSeparatorAt(const std::string &text, std::size_t position, const std::string &separator)
{
    return !separator.empty() && (text.compare(position, separator.size(), separator) == 0);
}

} // namespace

namespace act::text::CsvUtil
{

std::string EscapeCsvValue(const std::string &value, const std::string &separator)
{
    /* An empty separator is found at every position: there is no separator to protect */
    const bool hasSeparator {!separator.empty() && (value.find(separator) != std::string::npos)};
    const bool hasQuotingTriggerChars {value.find_first_of(CSV_QUOTING_TRIGGER_CHARS) != std::string::npos};
    /* ParseCsvLine trims the values which are not quoted: a surrounding whitespace has to be */
    const bool hasSurroundingWhitespace{!value.empty() &&
                                        (std::isspace(static_cast<unsigned char>(value.front())) ||
                                         std::isspace(static_cast<unsigned char>(value.back())))};
    if (!hasSeparator && !hasQuotingTriggerChars && !hasSurroundingWhitespace)
    {
        return value;
    }

    std::string escapedValue(1, CSV_QUOTE_CHAR);
    escapedValue.reserve(value.size() + 2);
    for (const char character : value)
    {
        if (character == CSV_QUOTE_CHAR)
        {
            /* A double quote inside a quoted value is doubled */
            escapedValue += CSV_QUOTE_CHAR;
        }
        escapedValue += character;
    }
    escapedValue += CSV_QUOTE_CHAR;

    return escapedValue;
}

std::string CreateCsvLine(const std::vector<std::string> &values, const std::string &separator)
{
    std::vector<std::string> escapedValues;
    escapedValues.reserve(values.size());
    for (const std::string &value : values)
    {
        escapedValues.push_back(EscapeCsvValue(value, separator));
    }

    return JoinCsvLine(escapedValues, separator);
}

std::string JoinCsvLine(const std::vector<std::string> &values, const std::string &separator)
{
    std::stringstream stream;
    stream << VectorStringUtil::join(values, separator);
    stream << std::endl;
    return stream.str();
}

bool AddCsvLine(const std::vector<std::string> &values,
                const act::foundation::AbsLogger &logger,
                act::files::ExtFile &file,
                const std::string &separator)
{
    if (!file.isOpen())
    {
        logger.warningStream() << "CsvUtil::AddCsvLine: File is not open: " << file.getFilePath();
        return false;
    }

    std::fstream &fs = file.accessFilePtr();

    fs << VectorStringUtil::join(values, separator);
    fs << std::endl;
    auto errorCode = fs.sync();
    if (errorCode != 0)
    {
        logger.warningStream() << "CsvUtil::AddCsvLine: Failed to sync file: " << file.getFilePath()
                               << ", after writing CSV line, error code: " << errorCode;
        return false;
    }

    return true;
}

std::vector<std::string> ParseCsvFile(const std::string &csvContent, const std::string &separator)
{
    std::vector<std::string> lines;
    std::string line;
    bool isInQuotes{false};
    bool isValueStart{true};

    /* A UTF-8 byte order mark at the very beginning is not a part of the content */
    using act::text::StringUtil::UTF8_BYTE_ORDER_MARK;
    std::size_t position{csvContent.starts_with(UTF8_BYTE_ORDER_MARK) ? UTF8_BYTE_ORDER_MARK.size()
                                                                      : 0};
    while (position < csvContent.size())
    {
        const char character{csvContent[position]};
        if (isInQuotes)
        {
            if (character == CSV_QUOTE_CHAR)
            {
                if (((position + 1) < csvContent.size()) &&
                    (csvContent[position + 1] == CSV_QUOTE_CHAR))
                {
                    /* A doubled double quote is a value character, not the end of the value */
                    line += character;
                    ++position;
                }
                else
                {
                    isInQuotes = false;
                }
            }
        }
        else if ((character == '\r') && ((position + 1) < csvContent.size()) &&
                 (csvContent[position + 1] == '\n'))
        {
            /* The carriage return of a CRLF line break is not a part of the line */
            ++position;
            continue;
        }
        else if (character == '\n')
        {
            lines.push_back(line);
            line.clear();
            isValueStart = true;
            ++position;
            continue;
        }
        else if (isValueStart && (character == CSV_QUOTE_CHAR))
        {
            isInQuotes = true;
            isValueStart = false;
        }
        else if (IsSeparatorAt(csvContent, position, separator))
        {
            isValueStart = true;
            line += separator;
            position += separator.size();
            continue;
        }
        else if (!std::isspace(static_cast<unsigned char>(character)))
        {
            isValueStart = false;
        }

        line += character;
        ++position;
    }

    if (!line.empty())
    {
        lines.push_back(line);
    }

    return lines;
}

void RemoveBlankLines(std::vector<std::string> &lines, const std::string &separator)
{
    std::erase_if(lines, [&separator](const std::string &line) {
        const std::vector<std::string> values{ParseCsvLine(line, separator)};
        return std::ranges::all_of(values, [](const std::string &value) { return value.empty(); });
    });
}

std::vector<std::string> ParseCsvLine(const std::string &csvLine, const std::string &separator)
{
    std::vector<std::string> values;
    if (csvLine.empty())
    {
        return values;
    }

    std::size_t position{0};
    bool hasNextValue{true};
    while (hasNextValue)
    {
        /* The whitespace before a value is skipped, but for the one the separator is made of */
        std::size_t valueStart{position};
        while ((valueStart < csvLine.size()) &&
               std::isspace(static_cast<unsigned char>(csvLine[valueStart])) &&
               !IsSeparatorAt(csvLine, valueStart, separator))
        {
            ++valueStart;
        }

        std::string value;
        if ((valueStart < csvLine.size()) && (csvLine[valueStart] == CSV_QUOTE_CHAR))
        {
            /* A value between double quotes is read as it is: it ends at the double quote which
             * is not doubled, or at the end of the line if there is none */
            position = valueStart + 1;
            bool isClosed{false};
            while ((position < csvLine.size()) && !isClosed)
            {
                if (csvLine[position] != CSV_QUOTE_CHAR)
                {
                    value += csvLine[position];
                    ++position;
                }
                else if (((position + 1) < csvLine.size()) &&
                         (csvLine[position + 1] == CSV_QUOTE_CHAR))
                {
                    value += CSV_QUOTE_CHAR;
                    position += 2;
                }
                else
                {
                    isClosed = true;
                    ++position;
                }
            }

            /* What follows the closing double quote, up to the next separator, is kept but for
             * its surrounding whitespace */
            std::string trailing;
            while ((position < csvLine.size()) && !IsSeparatorAt(csvLine, position, separator))
            {
                trailing += csvLine[position];
                ++position;
            }
            value += act::text::StringUtil::Trim(trailing);
        }
        else
        {
            /* A value which is not between double quotes ends at the next separator, and is
             * trimmed */
            position = valueStart;
            while ((position < csvLine.size()) && !IsSeparatorAt(csvLine, position, separator))
            {
                ++position;
            }
            value = csvLine.substr(valueStart, position - valueStart);
            act::text::StringUtil::Trim(value);
        }

        values.push_back(value);
        hasNextValue = (position < csvLine.size());
        if (hasNextValue)
        {
            position += separator.size();
        }
    }

    return values;
}

std::vector<std::optional<std::size_t>> FindColumns(const std::vector<std::string> &headerValues,
                                                    const std::vector<std::string> &titles)
{
    std::vector<std::optional<std::size_t>> positions;
    positions.reserve(titles.size());
    for (const std::string &title : titles)
    {
        const std::string trimmedTitle{StringUtil::TrimCopy(title)};
        std::optional<std::size_t> position;
        std::size_t matchCount{0};
        for (std::size_t index{0}; index < headerValues.size(); ++index)
        {
            if (StringUtil::AreStringEqualNoCase(StringUtil::TrimCopy(headerValues[index]),
                                                 trimmedTitle))
            {
                position = index;
                ++matchCount;
            }
        }

        /* A title found several times is ambiguous: it is not found either */
        positions.push_back((matchCount == 1) ? position : std::nullopt);
    }

    return positions;
}

} // namespace act::text::CsvUtil
