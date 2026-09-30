// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

namespace act::property
{

/**
 * @brief Who creates the table backing a SQLite property store.
 */
enum class TableProvisioning
{
    /**
     * @brief The store creates its table at init (`CREATE TABLE IF NOT EXISTS`), so it is
     *        self-contained. Fits a dedicated database, which then evolves through its own
     *        migrations.
     */
    SELF_CREATE,

    /**
     * @brief The table is created by the migrations of the database owner; the store only checks at
     *        init that it exists. Keeps a single source of truth for the schema.
     */
    EXTERNAL,
};

} // namespace act::property
