<!--
SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>

SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1
-->

# act_property_sqlite

A property store (`AbsPropertyStore` from `act_property_core`) persisting the properties in a
key/value table of a SQLite database.

One class covers both deployments, since a dedicated database is a key/value table too:

- **A table in the application database**: pass the protect service of the application database.
- **A dedicated database**: create a dedicated `ASqLiteDbManager` (its own file, slug and
  migrations) and pass its protect service. The store code is the same.

## Dependencies

- act_property_core
- act_db_core
- act_db_sqlite
- act_logger
- SQLiteCpp (libsqlitecpp-dev)

## Components

| Class/File            | Header                                                 | Role                                                   |
| --------------------- | ------------------------------------------------------ | ------------------------------------------------------ |
| `SqlitePropertyStore` | `act_property_sqlite/stores/sqlite_property_store.hpp` | Store backed by a key/value table of a SQLite database |
| `TableProvisioning`   | `act_property_sqlite/types/table_provisioning.hpp`     | Whether the store creates its table or expects it      |

## Table schema

The schema is generic: it never changes when a property is added.

```sql
CREATE TABLE IF NOT EXISTS "<table>" (
    "key"   TEXT PRIMARY KEY,
    "type"  INTEGER NOT NULL,
    "value"
) WITHOUT ROWID;
```

- `type` holds the `StoredType` tag of the value.
- `value` has no declared type, so it keeps the SQLite storage class it is written with: `INTEGER`
  for the booleans (0 or 1) and the integers, `REAL` for the floating point values, `TEXT` for the
  strings.
- A `UINT64` value is written as the two's complement reinterpretation of the value as a signed
  integer, since a SQLite integer is signed: an external reader sees a negative number for any
  value above `INT64_MAX`, and must reinterpret the column as unsigned when the tag is `UINT64`.

Reading is strict: a row whose tag is unknown, whose storage class does not match its tag, or whose
integer does not fit the tagged type reads as absent, and the mismatch is logged. SQLite stores a
NaN as `NULL`, so a NaN written to the store reads back as absent.

## Table provisioning

- `TableProvisioning::SELF_CREATE`: `init()` runs the `CREATE TABLE IF NOT EXISTS` above. The store
  is self-contained, which fits a dedicated database.
- `TableProvisioning::EXTERNAL`: `init()` only checks that the table exists, and fails otherwise.
  The migrations of the database owner create the table with the schema above, which keeps a single
  source of truth for the migrations.

The table name is always given by the application. It is quoted in every query, so any name is
accepted. Two stores may target two tables, or the same one.

## Usage

The store is a sub-service owned by a concrete property manager, which returns it from
`accessStore()`. The manager sub-logger is available to the member initializers, since the base is
constructed first.

```cpp
#include "act_property_core/properties/rw_property.hpp"
#include "act_property_core/services/abs_property_manager.hpp"
#include "act_property_sqlite/stores/sqlite_property_store.hpp"

class SettingsManager : public act::property::AbsPropertyManager
{
  public:
    SettingsManager(act::db::sqlite::SQLiteDbProtectService &db, act::logger::LoggerManager &logger)
        : AbsPropertyManager(logger),
          m_store(db, accessLogger(), "settings", act::property::TableProvisioning::EXTERNAL)
    {
    }

    act::property::RWProperty<int> brightness{*this, "ui.brightness", act::property::Default::of(50)};

  protected:
    act::property::AbsPropertyStore &accessStore() override
    {
        return m_store;
    }

  private:
    act::property::SqlitePropertyStore m_store;
};
```

No query opens a transaction: each one is a single statement, hence already atomic, and the store
stays usable while the caller holds a transaction on the same connection.

## CMake integration

```cmake
add_subdirectory(path/to/lib/act_property_sqlite act_property_sqlite)
target_link_libraries(my_target PRIVATE act_property_sqlite)
```
