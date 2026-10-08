// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_text/vector_string_util.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace act::text::VectorStringUtil
{

std::string join(const std::vector<std::string> &vec, const std::string &separator)
{
    auto citer = vec.cbegin();
    if (citer == vec.cend())
    {
        return "";
    }

    std::stringstream resultStream;
    resultStream << *citer;
    ++citer;
    if (separator.empty())
    {
        for (; citer != vec.cend(); ++citer)
        {
            resultStream << *citer;
        }
    }
    else
    {
        for (; citer != vec.cend(); ++citer)
        {
            resultStream << separator << *citer;
        }
    }

    return resultStream.str();
}

std::vector<std::string> split(const std::string &str, const std::string &separator)
{
    std::vector<std::string> tmpList;

    if (separator.empty())
    {
        // An empty separator is found at every position and would never advance the search
        if (!str.empty())
        {
            tmpList.push_back(str);
        }
        return tmpList;
    }

    if (!str.empty())
    {
        int start = 0;
        size_t idx = str.find(separator, start);
        while (idx != std::string::npos)
        {
            const int length = idx - start;
            tmpList.push_back(str.substr(start, length));
            start += (length + separator.size());

            idx = str.find(separator, start);
        }

        tmpList.push_back(str.substr(start));
    }

    return tmpList;
}

} // namespace act::text::VectorStringUtil
