<!--
SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>

SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1
-->

# act_property_core

Engine-agnostic library for persisting configuration between two runs: typed property descriptors,
codecs, value providers, the store interface and the property manager base.

An application declares its settings as typed objects ("properties"), grouped in a "property
manager", which reads and writes them through a "store". The store is an abstraction; this package
ships the interface and an in-memory implementation for tests. A concrete storage backend (for
example a SQLite backed store) lives in a companion package.

Design principles:

- The store is the only source of truth: no cache, every read goes to the store.
- Strong typing at the call site: a key is declared once, with its type.
- Two natures of default value, told apart by the provider type (not by a policy flag):
  - `Default`: a read fallback, never written; runtime writes are preserved.
  - `Seed`: an init value written to the store and realigned at each start, so external readers see
    it.

## Dependencies

- act_foundation
- act_logger

## Components

| Class/File              | Header                                                     | Role                                                                    |
| ----------------------- | ---------------------------------------------------------- | ----------------------------------------------------------------------- |
| `StoredValue`           | `act_property_core/stored_value.hpp`                       | Raw value with a closed set of native types and a type tag              |
| `ScalarCodec<T>`        | `act_property_core/codecs/scalar_codec.hpp`                | Codec for bool, fixed-width ints, float, double and `std::string`       |
| `StoredScalarTraits<T>` | `act_property_core/codecs/stored_scalar_traits.hpp`        | Per-type read accessor and `StoredScalar` concept                       |
| `EnumCodec<E>`          | `act_property_core/codecs/enum_codec.hpp`                  | Codec for an enum, stored by its name                                   |
| `SerializedCodec<T>`    | `act_property_core/codecs/serialized_codec.hpp`            | Codec storing an arbitrary object as a single string                    |
| `CodecFor<C, T>`        | `act_property_core/codecs/codec.hpp`                       | Compile-time concept a codec must satisfy for a value type              |
| `Default` / `Seed`      | `act_property_core/providers/*.hpp`                        | Value providers: read fallback vs init value written and realigned      |
| `AbsPropertyStore`      | `act_property_core/services/abs_property_store.hpp`        | Key -> typed value store interface (a sub-service)                      |
| `InMemoryPropertyStore` | `act_property_core/stores/in_memory_property_store.hpp`    | Volatile store, handy for tests                                         |
| `AbsPropertyManager`    | `act_property_core/services/abs_property_manager.hpp`      | Manager base: ties descriptors to one store, seeds and realigns at init |
| `AbsPropertyRegistry`   | `act_property_core/properties/abs_property_registry.hpp`   | What a descriptor needs from its owner: register, reach the store       |
| `AbsRegisteredProperty` | `act_property_core/properties/abs_registered_property.hpp` | What the owner needs from a descriptor: its key, seed it at init        |
| `RProperty<T>`          | `act_property_core/properties/r_property.hpp`              | Read only descriptor                                                    |
| `RWProperty<T>`         | `act_property_core/properties/rw_property.hpp`             | Read and write descriptor                                               |

## Usage

### 1. Declare a manager holding typed descriptors

A concrete manager derives from `AbsPropertyManager`, owns its store and returns it from
`accessStore()`. Each descriptor is declared once as a member. The manager hands itself to its
descriptors as an `AbsPropertyRegistry`, a protected base: outside the manager, the raw store stays
unreachable and no descriptor can be registered. In return, each descriptor hands itself to the
manager as an `AbsRegisteredProperty`, also a protected base: only the manager can seed it.

```cpp
#include "act_property_core/codecs/enum_codec.hpp"
#include "act_property_core/properties/r_property.hpp"
#include "act_property_core/properties/rw_property.hpp"
#include "act_property_core/services/abs_property_manager.hpp"
#include "act_property_core/stores/in_memory_property_store.hpp"

#include <cstdint>

enum class Locale
{
    EnGb,
    FrFr,
};

class SettingsManager : public act::property::AbsPropertyManager
{
  public:
    explicit SettingsManager(act::logger::LoggerManager &logger) : AbsPropertyManager(logger) {}

    // User data: a read fallback, never written, preserved across updates.
    act::property::RWProperty<int> brightness{*this, "ui.brightness", act::property::Default::of(50)};

    // The literal 80 is an int: name the property type explicitly (see below).
    act::property::RWProperty<std::uint8_t> volume{
        *this, "audio.volume", act::property::Default::of<std::uint8_t>(80)};

    // No provider: an absent key reads as an empty optional.
    act::property::RWProperty<std::string> lastPatientId{*this, "session.lastPatientId"};

    // Enum, stored by its name.
    act::property::RWProperty<Locale> uiLocale{
        *this,
        "ui.locale",
        act::property::Default::of(Locale::EnGb),
        act::property::EnumCodec<Locale>({{Locale::EnGb, "en_GB"}, {Locale::FrFr, "fr_FR"}})};

    // Code driven data: a seed written and realigned at init, read only at runtime.
    act::property::RProperty<int> maxRetries{*this, "net.maxRetries", act::property::Seed::of(3)};

  protected:
    act::property::AbsPropertyStore &accessStore() override
    {
        return m_store;
    }

  private:
    act::property::InMemoryPropertyStore m_store;
};
```

A provider carries the exact value type of its property and never converts to another one.
`Default::of(50)` yields an `int` provider: it fits an `RProperty<int>`, but not an
`RProperty<std::uint8_t>`, an `RProperty<std::int64_t>` or, for `Default::of(0.5)`, an
`RProperty<float>`, and `Default::of("fr_FR")` does not fit an `RProperty<std::string>`. Those do
not compile; name the type instead, as in `Default::of<std::uint8_t>(80)`. The conversion then
happens at the call site, where the compiler warns when a constant does not fit (`300` for a
`std::uint8_t`). With `Default::from` or `Seed::from`, give the function the exact return type,
for example `[]() -> std::uint8_t { return 80; }`.

### 2. Initialize, then read and write

```cpp
SettingsManager settings(logger);
settings.init(); // initializes the store, then seeds and realigns every Seed descriptor

const int b = settings.brightness.getOr(50);   // store first, else the Default fallback
settings.brightness.set(80);                    // persisted

const int retries = settings.maxRetries.getOr(3); // already materialized by init
```

### 3. Store a complex object with a serialized codec

`act_property_core` holds no serialization dependency: supply the two conversion callables. Any
encoding works (JSON, CSV, a hand rolled format, ...); the object is stored as one string.

```cpp
#include "act_property_core/codecs/serialized_codec.hpp"

struct ContactInfo
{
    std::string firstName;
    std::string lastName;
};

act::property::RWProperty<ContactInfo> contactInfo{
    *this,
    "local.contactInfo",
    act::property::serializedCodec<ContactInfo>(
        [](const ContactInfo &c) { return serialize(c); },
        [](const std::string &s) { return parse(s); })};
```

## CMake integration

```cmake
add_subdirectory(path/to/lib/act_property_core act_property_core)
target_link_libraries(my_target PRIVATE act_property_core)
```
