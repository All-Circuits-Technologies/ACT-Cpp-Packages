// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <cstdio>
#include <string>
#include <vector>

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

namespace act::files
{
class ExtFile;
} // namespace act::files

namespace act::text::CsvUtil
{
/** @brief Default CSV separator character */
const constexpr char *DefaultCsvSeparator = ",";

/**
 * @brief Create a CSV line from a vector of strings
 * @param values The values to include in the CSV line
 * @param separator The separator character (default is @ref DefaultCsvSeparator)
 * @return The created CSV line as a string
 */
std::string CreateCsvLine(const std::vector<std::string> &values,
                          const std::string &separator = DefaultCsvSeparator);

/**
 * @brief Add a line to a CSV file
 * @param values The values to add
 * @param logger The logger to use for logging
 * @param file The file to write to
 * @param separator The separator character (default is @ref DefaultCsvSeparator)
 * @return true upon success, false otherwise
 */
bool AddCsvLine(const std::vector<std::string> &values,
                const act::foundation::AbsLogger &logger,
                act::files::ExtFile &file,
                const std::string &separator = DefaultCsvSeparator);

/**
 * @brief Parse CSV content into lines
 * @param csvContent The CSV content as a string
 * @return A vector of strings, each representing a line from the CSV content
 */
std::vector<std::string> ParseCsvFile(const std::string &csvContent);

/**
 * @brief Parse a single CSV line into values
 * @param csvLine The CSV line as a string
 * @param separator The separator character (default is @ref DefaultCsvSeparator)
 * @return A vector of strings, each representing a value from the CSV line
 */
std::vector<std::string> ParseCsvLine(const std::string &csvLine,
                                      const std::string &separator = DefaultCsvSeparator);
} // namespace act::text::CsvUtil
