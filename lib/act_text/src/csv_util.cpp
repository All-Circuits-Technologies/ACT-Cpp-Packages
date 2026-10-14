// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_text/csv_util.hpp"

#include "act_files/ext_file.hpp"
#include "act_foundation/logger/abs_logger.hpp"
#include "act_text/string_util.hpp"
#include "act_text/vector_string_util.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

namespace act::text::CsvUtil
{

std::string EscapeCsvValue(const std::string &value, const std::string &separator)
{
    /* An empty separator is found at every position: there is no separator to protect */
    const bool hasSeparator {!separator.empty() && (value.find(separator) != std::string::npos)};
    const bool hasQuotingTriggerChars {value.find_first_of(CSV_QUOTING_TRIGGER_CHARS) != std::string::npos};
    if (!hasSeparator && !hasQuotingTriggerChars)
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

std::vector<std::string> ParseCsvFile(const std::string &csvContent)
{
    std::vector<std::string> lines;
    std::stringstream streamContent(csvContent);
    std::string line;

    while (std::getline(streamContent, line))
    {
        lines.push_back(line);
    }

    return lines;
}

std::vector<std::string> ParseCsvLine(const std::string &csvLine, const std::string &separator)
{
    auto values = VectorStringUtil::split(csvLine, separator);
    std::vector<std::string> trimmedValues;
    trimmedValues.reserve(values.size());
    for (auto &value : values)
    {
        /* Trim whitespace from value */
        trimmedValues.push_back(act::text::StringUtil::Trim(value));
    }

    return trimmedValues;
}

} // namespace act::text::CsvUtil
