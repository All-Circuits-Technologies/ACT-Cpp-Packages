/**
 * SPDX-FileCopyrightText: 2025 Ghislain Mangé <ghislain.mange@allcircuits.com>
 *
 * SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1
 */

#include "act_files/file_util.hpp"

#include "act_files/ext_file.hpp"
#include "act_logger/models/abs_logger.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

namespace
{

/** @brief Byte order mark an UTF-8 file may start with */
constexpr std::string_view UTF8_BYTE_ORDER_MARK = "\xEF\xBB\xBF";

/** @brief Whitespace characters, as std::isspace in the default locale */
constexpr const char *WHITESPACE_CHARS = " \t\n\v\f\r";

} // namespace

namespace act::files::FileUtil
{
std::optional<std::string> ReadFile(const std::string &path,
                                    const act::logger::AbsLogger &logger,
                                    std::ios::openmode mode)
{
    std::ifstream file(path, mode);
    if (!file.is_open())
    {
        /**
         * use debug, not error:
         * full path should not be printed on embedded production run
         */
        logger.debugStream() << "Failed to open " << path;
        return std::nullopt;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::optional<int> ReadFileAsInt(const std::string &path,
                                 const act::logger::AbsLogger &logger,
                                 std::ios::openmode mode)
{
    auto optContent = ReadFile(path, logger, mode);
    if (!optContent.has_value())
    {
        return std::nullopt;
    }

    std::string content = std::move(optContent.value());
    if (content.starts_with(UTF8_BYTE_ORDER_MARK))
    {
        content.erase(0, UTF8_BYTE_ORDER_MARK.size());
    }

    // Trailing whitespace is accepted, as the line feed ending the files of sysfs; std::stoi
    // already skips the leading one
    const auto lastNonSpace = content.find_last_not_of(WHITESPACE_CHARS);
    content.erase((lastNonSpace == std::string::npos) ? 0 : (lastNonSpace + 1));

    int value = 0;
    std::size_t parsedLength = 0;
    try
    {
        value = std::stoi(content, &parsedLength);
    }
    catch (const std::exception &)
    {
        parsedLength = 0;
    }

    if (parsedLength == 0 || parsedLength != content.size())
    {
        // Nothing parsed, or characters left after the number ("12abc", an UTF-16 file...)
        logger.errorStream() << "Failed to parse " << content << " as int";
        return std::nullopt;
    }

    return value;
}

bool WriteFile(const std::string &path,
               const std::string &content,
               const act::logger::AbsLogger &logger,
               std::ios::openmode mode)
{
    std::ofstream file(path, mode);
    if (!file.is_open())
    {
        /**
         * use debug, not error:
         * full path should not be printed on embedded production run
         */
        logger.debugStream() << "Failed to open " << path;
        return false;
    }

    file << content;
    file.close();

    if (!file.good())
    {
        /**
         * use debug, not error:
         * full path should not be printed on embedded production run
         */
        logger.debugStream() << "Failed to write to " << path;
        return false;
    }

    return true;
}

std::shared_ptr<ExtFile> CreateFile(const std::string &path,
                                    const act::logger::AbsLogger &logger,
                                    std::ios::openmode mode,
                                    bool isTemp)
{
    auto extFile = ExtFile::CreateFileAndTryToOpenIt(path, mode, logger, isTemp);
    if (extFile == nullptr)
    {
        return nullptr;
    }

    return std::shared_ptr<ExtFile>(extFile);
}

std::shared_ptr<ExtFile> CreateFile(const std::string &path,
                                    const act::logger::AbsLogger &logger,
                                    bool isTemp)
{
    auto extFile = new ExtFile(path, logger, isTemp);
    return std::shared_ptr<ExtFile>(extFile);
}

bool ArePathsEqual(const std::string &path1,
                   const std::string &path2,
                   const act::logger::AbsLogger &logger)
{
    if (path1 == path2)
    {
        // No need to check further if the strings are identical
        return true;
    }

    bool isEqual = false;
    try
    {
        auto path1Obj = std::filesystem::absolute(path1);
        auto path2Obj = std::filesystem::absolute(path2);
        isEqual = std::filesystem::equivalent(path1, path2);
    }
    catch (const std::exception &e)
    {
        logger.warningStream() << "IsPathEqual: Exception occurred while comparing paths '" << path1
                               << "' and '" << path2 << "': " << e.what();
    }

    return isEqual;
}

std::string GetFilename(const std::string &path)
{
    if (path.empty())
    {
        return "";
    }

    std::filesystem::path fsPath(path);
    return fsPath.filename().string();
}
} /* namespace act::files::FileUtil */
