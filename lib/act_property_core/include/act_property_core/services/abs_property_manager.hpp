// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#pragma once

#include "act_foundation/services/abs_manager.hpp"
#include "act_property_core/properties/abs_property_registry.hpp"

#include <memory>
#include <vector>

namespace act::logger
{
class AbsLogger;
class LoggerManager;
} // namespace act::logger

namespace act::property
{

/**
 * @brief Base of a property manager: it ties a set of descriptors to a single store.
 *
 * The manager is a first level unit orchestrated by the global manager (it derives from
 * @ref act::foundation::AbsManager). The store, on the other hand, is a sub-service owned by the
 * concrete manager, which creates and initializes it; the global manager only ever sees the
 * manager.
 *
 * Descriptors only know the manager through the @ref AbsPropertyRegistry capability, inherited as a
 * protected base: a concrete manager can hand `*this` to the descriptors it declares as members,
 * while code holding a reference to the manager can neither reach the raw store nor register
 * descriptors of its own. Each descriptor registers itself at construction and reaches the store
 * lazily through @ref AbsPropertyRegistry::accessStore, only at init and at access time. That
 * indirection resolves the construction order trap: the base holds no reference to a store member
 * that may not be constructed yet, it asks for the store once it exists. The concrete manager owns
 * the store and provides @ref AbsPropertyRegistry::accessStore.
 *
 * In the other direction, the manager only knows a descriptor through the
 * @ref AbsRegisteredProperty capability, which the descriptor hands over at registration: the
 * manager seeds every descriptor at init without depending on the descriptor hierarchy, and no one
 * else can trigger that seeding.
 */
class AbsPropertyManager : public act::foundation::AbsManager, protected AbsPropertyRegistry
{
  protected:
    /**
     * @brief Construct the manager
     * @param logger The logger manager used to create the manager sub-logger
     */
    explicit AbsPropertyManager(act::logger::LoggerManager &logger);

  public:
    /// @brief Nothing special for default destructor
    ~AbsPropertyManager() override = default;

    /**
     * @brief Initialize the store, then seed and realign every registered descriptor.
     * @return true if the store initialized and every seed was materialized, false otherwise
     */
    bool init() override;

  protected:
    /**
     * @brief Access the manager sub-logger
     * @return The logger
     */
    [[nodiscard]] act::logger::AbsLogger &accessLogger() const;

  private:
    /** @copydoc AbsPropertyRegistry::registerProperty */
    void registerProperty(AbsRegisteredProperty &property) override;

  private:
    /** @brief The registered descriptors (not owned) */
    std::vector<AbsRegisteredProperty *> m_properties;

    /** @brief The manager sub-logger */
    std::shared_ptr<act::logger::AbsLogger> m_logger;
};

} // namespace act::property
