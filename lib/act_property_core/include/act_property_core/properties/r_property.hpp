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

#include "act_foundation/logger/abs_logger.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace act::property
{

/**
 * @brief A read only descriptor: a key, a type `T`, a codec and an optional value provider.
 *
 * Declared once as a member of its owner (typically a property manager). A read always goes to the
 * store first; when the key is absent, or when its stored value cannot be decoded (which is logged
 * as a warning), a @ref DefaultProvider (if any) supplies the fallback, while a @ref SeedProvider
 * is never consulted on read (it was already materialized at init). The codec is a concrete
 * compile-time type; it is shared by two type-erased callables so that a single template parameter
 * `T` carries the descriptor, whatever the codec.
 *
 * @tparam T The application type of the value
 */
template <class T>
class RProperty : public AbsPropertyBase
{
  public:
    /** @brief Encoder: application value -> stored value, empty if the value cannot be encoded */
    using Encoder = std::function<std::optional<StoredValue>(const T &)>;

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
        : RProperty(registry,
                    std::move(key),
                    std::make_shared<const CodecT>(std::move(codec)),
                    std::nullopt,
                    std::nullopt)
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
        : RProperty(registry,
                    std::move(key),
                    std::make_shared<const CodecT>(std::move(codec)),
                    std::move(fallback),
                    std::nullopt)
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
        : RProperty(registry,
                    std::move(key),
                    std::make_shared<const CodecT>(std::move(codec)),
                    std::nullopt,
                    std::move(seed))
    {
    }

  private:
    /**
     * @brief Build a descriptor around a codec shared by its encoder and decoder
     * @tparam CodecT The codec type
     * @param registry The owning registry
     * @param key The key of this descriptor
     * @param codec The codec converting between `T` and a stored value
     * @param fallback The read fallback provider, if any
     * @param seed The seed provider, if any
     */
    template <class CodecT>
        requires CodecFor<CodecT, T>
    RProperty(AbsPropertyRegistry &registry,
              std::string key,
              const std::shared_ptr<const CodecT> &codec,
              std::optional<DefaultProvider<T>> fallback,
              std::optional<SeedProvider<T>> seed)
        : AbsPropertyBase(registry, std::move(key)),
          m_encode(MakeEncoder(codec)),
          m_decode(MakeDecoder(codec)),
          m_fallback(std::move(fallback)),
          m_seed(std::move(seed))
    {
    }

  public:
    /**
     * @brief Read the value, always from the store first.
     * @note A stored value which cannot be decoded is logged as a warning and reads as absent.
     * @return The stored value decoded, the read fallback if the key is absent (or its value cannot
     *         be decoded) and a default provider is set, or an empty optional otherwise.
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
     * @brief Encode a value with the codec of this property.
     * @note An encode failure is logged as an error.
     * @param value The value to encode
     * @return The stored value, or an empty optional if the codec cannot encode @p value
     */
    [[nodiscard]] std::optional<StoredValue> encode(const T &value) const;

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
     * @brief Wrap a shared concrete codec into a type-erased encoder
     * @tparam CodecT The codec type
     * @param codec The codec to wrap, shared with the decoder
     * @return The encoder
     */
    template <class CodecT>
        requires CodecFor<CodecT, T>
    [[nodiscard]] static Encoder MakeEncoder(const std::shared_ptr<const CodecT> &codec);

    /**
     * @brief Wrap a shared concrete codec into a type-erased decoder
     * @tparam CodecT The codec type
     * @param codec The codec to wrap, shared with the encoder
     * @return The decoder
     */
    template <class CodecT>
        requires CodecFor<CodecT, T>
    [[nodiscard]] static Decoder MakeDecoder(const std::shared_ptr<const CodecT> &codec);

  private:
    /** @brief The encoder wrapping the concrete codec, which it shares with the decoder */
    Encoder m_encode;

    /** @brief The decoder wrapping the concrete codec, which it shares with the encoder */
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
        std::optional<T> decoded = m_decode(*raw);
        if (decoded.has_value())
        {
            return decoded;
        }

        accessLogger().warningStream() << "The stored value of property '" << getKey()
                                       << "' cannot be decoded, it reads as absent";
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
[[nodiscard]] std::optional<StoredValue> RProperty<T>::encode(const T &value) const
{
    std::optional<StoredValue> encoded = m_encode(value);
    if (!encoded.has_value())
    {
        accessLogger().errorStream()
            << "The value of property '" << getKey() << "' cannot be encoded";
    }

    return encoded;
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

    const std::optional<StoredValue> desired = encode(m_seed->evaluate());
    if (!desired.has_value())
    {
        return false;
    }

    AbsPropertyStore &store = accessStore();
    const std::optional<StoredValue> current = store.get(getKey());
    if (current.has_value() && *current == *desired)
    {
        // Already aligned: skip the write to spare flash wear.
        return true;
    }

    return store.set(getKey(), *desired);
}

template <typename T>
template <class CodecT>
    requires CodecFor<CodecT, T>
[[nodiscard]] typename RProperty<T>::Encoder RProperty<T>::MakeEncoder(
    const std::shared_ptr<const CodecT> &codec)
{
    return [codec](const T &value) -> std::optional<StoredValue> { return codec->encode(value); };
}

template <typename T>
template <class CodecT>
    requires CodecFor<CodecT, T>
[[nodiscard]] typename RProperty<T>::Decoder RProperty<T>::MakeDecoder(
    const std::shared_ptr<const CodecT> &codec)
{
    return [codec](const StoredValue &raw) -> std::optional<T> { return codec->decode(raw); };
}

} // namespace act::property
