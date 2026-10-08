// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include <cstdio>
#include <string>
#include <vector>

/** @brief Contains the string list utility functions */
namespace act::text::VectorStringUtil
{
/**
 * @brief Join the elements of a vector of strings into a single string with a separator.
 * @param vec The vector of strings to join.
 * @param separator The separator to insert between elements. Default is an empty string.
 * @return The joined string.
 */
std::string join(const std::vector<std::string> &vec, const std::string &separator = "");

/**
 * @brief Split a string into a vector of strings based on a separator.
 * @param str The string to split.
 * @param separator The separator to use for splitting. An empty separator does not split: the
 *                  whole string is returned as the only element.
 * @return A vector of strings resulting from the split, empty if @p str is empty.
 */
std::vector<std::string> split(const std::string &str, const std::string &separator);
} // namespace act::text::VectorStringUtil
