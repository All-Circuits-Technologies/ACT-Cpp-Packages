// SPDX-FileCopyrightText: 2025 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/not_copiable_not_movable.hpp"

#include <cstdio>
#include <fstream>
#include <optional>
#include <string>

namespace act::foundation
{
class AbsLogger;
} // namespace act::foundation

namespace act::files
{

/**
 * @brief This defines an useful file class to act on file and get its path
 * @note The class owns its file stream, so it can neither be copied nor moved
 */
class ExtFile : private act::foundation::NotCopiableNotMovable
{
  public:
    /**
     * @brief Default constructor
     * @param filePath The file path
     * @param logger The logger instance
     * @param isTemp Whether the file is temporary and should be deleted upon closing
     */
    explicit ExtFile(std::string filePath,
                     const act::foundation::AbsLogger &logger,
                     bool isTemp = false);

    /**
     * @brief Default destructor
     */
    virtual ~ExtFile();

  private:
    /**
     * @brief Constructor with fstream pointer
     * @param fstream The file stream pointer, we expect the pointer to be valid
     * @param filePath The file path
     * @param logger The logger instance
     * @param isTemp Whether the file is temporary and should be deleted upon closing
     */
    explicit ExtFile(std::fstream *fstream,
                     std::string filePath,
                     const act::foundation::AbsLogger &logger,
                     std::ios::openmode mode,
                     bool isTemp = false);

  public:
    /**
     * @brief Get the fstream pointer, to write, read or manipulate the file
     * @note To open or close the file, prefere to use open() and close() methods of this class.
     *       They will update the internal state accordingly.
     * @return The fstream reference
     */
    [[nodiscard]] std::fstream &accessFilePtr() const
    {
        return *m_fstream;
    }

    /** @brief Get the file path */
    [[nodiscard]] const std::string &getFilePath() const
    {
        return m_filePath;
    }

    /** @brief Get absolute file path */
    [[nodiscard]] std::string getAbsoluteFilePath() const;

    /** @brief Check if the file is temporary */
    [[nodiscard]] bool isTemporary() const
    {
        return m_isTemp;
    }

    /**
     * @brief Open the file with the given mode
     * @param mode The file open mode
     * @return True on success, false on failure
     */
    [[nodiscard]] bool open(std::ios::openmode mode);

    /** @brief Close the file */
    void close();

    /** @brief Check if the file is open */
    [[nodiscard]] bool isOpen() const;

  public:
    /**
     * @brief Create an ExtFile instance and try to open it
     * @param filePath The file path
     * @param mode The file open mode
     * @param logger The logger instance
     * @param isTemp Whether the file is temporary and should be deleted upon closing
     * @return The created ExtFile instance, or nullptr upon failure
     */
    static ExtFile *CreateFileAndTryToOpenIt(const std::string &filePath,
                                             std::ios::openmode mode,
                                             const act::foundation::AbsLogger &logger,
                                             bool isTemp = false);

  private:
    /**
     * @brief Open a file with the given mode
     * @param fstream The file stream to open
     * @param filePath The file path
     * @param mode The file open mode
     * @param logger The logger instance
     * @return True on success, false on failure
     */
    static bool OpenFile(std::fstream &fstream,
                         const std::string &filePath,
                         std::ios::openmode mode,
                         const act::foundation::AbsLogger &logger);

  private:
    /** @brief Logger instance */
    const act::foundation::AbsLogger &m_logger;

    /** @brief This is the pointer to the file stream  */
    std::fstream *m_fstream{nullptr};

    /**
     * @brief The current file open mode
     * @note This is std::nullopt if the file is not opened
     */
    std::optional<std::ios::openmode> m_mode;

    /** @brief The file path */
    std::string m_filePath;

    /** @brief Whether the file is temporary and should be deleted upon closing */
    bool m_isTemp{false};
};

} // namespace act::files
