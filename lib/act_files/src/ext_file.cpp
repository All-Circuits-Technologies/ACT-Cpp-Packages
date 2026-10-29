// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_files/ext_file.hpp"

#include "act_foundation/logger/abs_logger.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ios>
#include <optional>
#include <string>
#include <utility>

namespace act::files
{

ExtFile::ExtFile(std::string filePath, const act::foundation::AbsLogger &logger, bool isTemp)
    : m_logger{logger},
      m_fstream{new std::fstream()},
      m_filePath(std::move(filePath)),
      m_isTemp(isTemp)
{
}

ExtFile::~ExtFile()
{
    m_fstream->close();

    if (m_isTemp)
    {
        if (std::remove(m_filePath.c_str()) != 0)
        {
            m_logger.warningStream() << "Failed to delete temporary file '" << m_filePath << "'";
        }
    }

    delete m_fstream;
}

ExtFile::ExtFile(std::fstream *fstream,
                 std::string filePath,
                 const act::foundation::AbsLogger &logger,
                 std::ios::openmode mode,
                 bool isTemp)
    : m_logger{logger},
      m_fstream{fstream},
      m_mode{mode},
      m_filePath(std::move(filePath)),
      m_isTemp(isTemp)
{
}

std::string ExtFile::getAbsoluteFilePath() const
{
    return std::filesystem::absolute(m_filePath).string();
}

bool ExtFile::open(std::ios::openmode mode)
{
    if (m_mode.has_value() && m_mode.value() == mode)
    {
        // File is already opened with the requested mode
        // Nothing to do
        return true;
    }

    if (m_fstream->is_open())
    {
        // std::fstream::open fails on an open stream; closing also flushes what was written
        close();
    }

    auto openResult = OpenFile(*m_fstream, m_filePath, mode, m_logger);
    if (!openResult)
    {
        return false;
    }

    m_mode = mode;

    return true;
}

void ExtFile::close()
{
    // Close method already test if the file is opened
    m_fstream->close();
    m_mode = std::nullopt;
}

bool ExtFile::isOpen() const
{
    return m_fstream->is_open();
}

ExtFile *ExtFile::CreateFileAndTryToOpenIt(const std::string &filePath,
                                           std::ios::openmode mode,
                                           const act::foundation::AbsLogger &logger,
                                           bool isTemp)
{
    auto *fstream = new std::fstream();
    auto openResult = OpenFile(*fstream, filePath, mode, logger);
    if (!openResult)
    {
        delete fstream;
        return nullptr;
    }

    return new ExtFile(fstream, filePath, logger, mode, isTemp);
}

bool ExtFile::OpenFile(std::fstream &fstream,
                       const std::string &filePath,
                       std::ios::openmode mode,
                       const act::foundation::AbsLogger &logger)
{
    fstream.open(filePath, mode);
    if (!fstream.is_open())
    {
        logger.errorStream() << "Failed to open file '" << filePath << "'";
        return false;
    }

    return true;
}

} // namespace act::files
