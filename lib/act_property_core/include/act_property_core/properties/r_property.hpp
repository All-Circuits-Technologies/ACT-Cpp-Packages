// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_property_core/codecs/codec.hpp"
#include "act_property_core/codecs/scalar_codec.hpp"
#include "act_property_core/properties/abs_property_base.hpp"
#include "act_property_core/providers/default_provider.hpp"
#include "act_property_core/providers/seed_provider.hpp"
#include "act_property_core/services/abs_property_store.hpp"
#include "act_property_core/stored_value.hpp"

#include <functional>
#include <optional>
#include <string>
#include <utility>

namespace act::property
{

/**
 * @brief A read only descriptor: a key, a type `T`, a codec and an optional value provider.
 *
 * Declared once as a member of its owner (typically a property manager). A read always goes to the
 * store first; when the key is absent, a @ref DefaultProvider (if any) supplies the fallback, while
 * a @ref SeedProvider is never consulted on read (it was already materialized at init). The codec
 * is a concrete compile-time type; it is wrapped here into type-erased callables so that a single
 * template parameter `T` carries the descriptor, whatever the codec.
 *
 * @tparam T The application type of the value
 */
template <class T>
class RProperty : public AbsPropertyBase
{
  public:
    /** @brief Encoder: application value -> stored value */
    using Encoder = std::function<StoredValue(const T &)>;

    /** @brief Decoder: stored value -> application value, empty on tag mismatch or parse failure */
    using Decoder = std::function<std::optional<T>(const StoredValue &)>;

  public:
    /**
     * @brief Build a scalar descriptor with no value provider
     * @param registry The owning registry
     * @param key The key of this descriptor
     */
    RProperty(AbsPropertyRegistry &registry, std::string key)
        : RProperty(registry, std::move(key), ScalarCodec<T>{})
    {
    }

    /**
     * @brief Build a scalar descriptor with a read fallback
     * @param registry The owning registry
     * @param key The key of this descriptor
     * @param fallback The read fallback value
     */
    RProperty(AbsPropertyRegistry &registry, std::string key, DefaultProvider<T> fallback)
        : RProperty(registry, std::move(key), std::move(fallback), ScalarCodec<T>{})
    {
    }

    /**
     * @brief Build a scalar descriptor with a seed
     * @param registry The owning registry
     * @param key The key of this descriptor
     * @param seed The init value written and realigned at each start
     */
    RProperty(AbsPropertyRegistry &registry, std::string key, SeedProvider<T> seed)
        : RProperty(registry, std::move(key), std::move(seed), ScalarCodec<T>{})
    {
    }

    /**
     * @brief Build a descriptor with a custom codec and no value provider
     * @tparam CodecT The codec type
     * @param registry The owning registry
     * @param key The key of this descriptor
     * @param codec The codec converting between `T` and a stored value
     */
    template <class CodecT>
        requires CodecFor<CodecT, T>
    RProperty(AbsPropertyRegistry &registry, std::string key, CodecT codec)
        : AbsPropertyBase(registry, std::move(key)),
          m_encode(MakeEncoder(codec)),
          m_decode(MakeDecoder(std::move(codec)))
    {
    }

    /**
     * @brief Build a descriptor with a read fallback and a custom codec
     * @tparam CodecT The codec type
     * @param registry The owning registry
     * @param key The key of this descriptor
     * @param fallback The read fallback value
     * @param codec The codec converting between `T` and a stored value
     */
    template <class CodecT>
        requires CodecFor<CodecT, T>
    RProperty(AbsPropertyRegistry &registry,
              std::string key,
              DefaultProvider<T> fallback,
              CodecT codec)
        : AbsPropertyBase(registry, std::move(key)),
          m_encode(MakeEncoder(codec)),
          m_decode(MakeDecoder(std::move(codec))),
          m_fallback(std::move(fallback))
    {
    }

    /**
     * @brief Build a descriptor with a seed and a custom codec
     * @tparam CodecT The codec type
     * @param registry The owning registry
     * @param key The key of this descriptor
     * @param seed The init value written and realigned at each start
     * @param codec The codec converting between `T` and a stored value
     */
    template <class CodecT>
        requires CodecFor<CodecT, T>
    RProperty(AbsPropertyRegistry &registry, std::string key, SeedProvider<T> seed, CodecT codec)
        : AbsPropertyBase(registry, std::move(key)),
          m_encode(MakeEncoder(codec)),
          m_decode(MakeDecoder(std::move(codec))),
          m_seed(std::move(seed))
    {
    }

  public:
    /**
     * @brief Read the value, always from the store first.
     * @return The stored value decoded, the read fallback if the key is absent and a default
     *         provider is set, or an empty optional otherwise (including on a decode failure of a
     *         present value).
     */
    [[nodiscard]] std::optional<T> get() const;

    /**
     * @brief Read the value or a caller supplied fallback
     * @param fallback The value returned when @ref get yields nothing
     * @return The read value, or @p fallback
     */
    [[nodiscard]] T getOr(const T &fallback) const;

  protected:
    /**
     * @brief Get the Encoder object associated with this property.
     *
     * @return The encoder associated with this property.
     */
    [[nodiscard]] const Encoder &getEncoder() const;

    /**
     * @brief Get the Fallback object associated with this property.
     *
     * @return The fallback provider associated with this property, if any.
     */
    [[nodiscard]] const std::optional<DefaultProvider<T>> &getFallback() const;

    /**
     * @brief Get the Seed object associated with this property.
     *
     * @return The seed provider associated with this property, if any.
     */
    [[nodiscard]] const std::optional<SeedProvider<T>> &getSeed() const;

  private:
    /**
     * @brief Materialize the seed into the store when the key is absent or the seed changed.
     * @note Private: only the registry reaches it, through @ref AbsRegisteredProperty.
     * @return true when nothing had to be written or the write succeeded, false otherwise
     */
    bool seedIntoStore() override;

  private:
    /**
     * @brief Wrap a concrete codec into a type-erased encoder
     * @tparam CodecT The codec type
     * @param codec The codec to wrap
     * @return The encoder
     */
    template <class CodecT>
        requires CodecFor<CodecT, T>
    [[nodiscard]] static Encoder MakeEncoder(CodecT codec);

    /**
     * @brief Wrap a concrete codec into a type-erased decoder
     * @tparam CodecT The codec type
     * @param codec The codec to wrap
     * @return The decoder
     */
    template <class CodecT>
        requires CodecFor<CodecT, T>
    [[nodiscard]] static Decoder MakeDecoder(CodecT codec);

  private:
    /** @brief The encoder wrapping the concrete codec */
    Encoder m_encode;

    /** @brief The decoder wrapping the concrete codec */
    Decoder m_decode;

    /** @brief The read fallback provider, set only for a default backed descriptor */
    std::optional<DefaultProvider<T>> m_fallback;

    /** @brief The seed provider, set only for a seeded descriptor */
    std::optional<SeedProvider<T>> m_seed;
};

template <typename T>
[[nodiscard]] std::optional<T> RProperty<T>::get() const
{
    const std::optional<StoredValue> raw = accessStore().get(getKey());
    if (raw.has_value())
    {
        return m_decode(*raw);
    }

    if (m_fallback.has_value())
    {
        return m_fallback->evaluate();
    }

    return std::nullopt;
}

template <typename T>
[[nodiscard]] T RProperty<T>::getOr(const T &fallback) const
{
    return get().value_or(fallback);
}

template <typename T>
[[nodiscard]] const typename RProperty<T>::Encoder &RProperty<T>::getEncoder() const
{
    return m_encode;
}

template <typename T>
[[nodiscard]] const std::optional<DefaultProvider<T>> &RProperty<T>::getFallback() const
{
    return m_fallback;
}

template <typename T>
[[nodiscard]] const std::optional<SeedProvider<T>> &RProperty<T>::getSeed() const
{
    return m_seed;
}

template <typename T>
bool RProperty<T>::seedIntoStore()
{
    if (!m_seed.has_value())
    {
        // Default or no provider: never write at init.
        return true;
    }

    const StoredValue desired = m_encode(m_seed->evaluate());
    AbsPropertyStore &store = accessStore();
    const std::optional<StoredValue> current = store.get(getKey());
    if (current.has_value() && *current == desired)
    {
        // Already aligned: skip the write to spare flash wear.
        return true;
    }

    return store.set(getKey(), desired);
}

template <typename T>
template <class CodecT>
    requires CodecFor<CodecT, T>
[[nodiscard]] typename RProperty<T>::Encoder RProperty<T>::MakeEncoder(CodecT codec)
{
    return
        [codec = std::move(codec)](const T &value) -> StoredValue { return codec.encode(value); };
}

template <typename T>
template <class CodecT>
    requires CodecFor<CodecT, T>
[[nodiscard]] typename RProperty<T>::Decoder RProperty<T>::MakeDecoder(CodecT codec)
{
    return [codec = std::move(codec)](const StoredValue &raw) -> std::optional<T> {
        return codec.decode(raw);
    };
}

} // namespace act::property
