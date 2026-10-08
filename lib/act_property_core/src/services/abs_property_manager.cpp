// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_property_core/services/abs_property_manager.hpp"

#include "act_property_core/properties/abs_registered_property.hpp"
#include "act_property_core/services/abs_property_store.hpp"

#include "act_foundation/logger/abs_logger.hpp"
#include "act_logger/services/logger_manager.hpp"

namespace act::property
{

namespace
{
    /** @brief Sub-logger category for a property manager */
    constexpr const char *LoggerCategory = "property";
} // namespace

AbsPropertyManager::AbsPropertyManager(act::logger::LoggerManager &logger)
    : act::foundation::AbsManager(),
      m_logger(logger.createSubLogger(LoggerCategory))
{
}

bool AbsPropertyManager::init()
{
    AbsPropertyStore &store = accessStore();
    if (!store.init())
    {
        m_logger->error("Failed to initialize the property store");
        return false;
    }

    for (AbsRegisteredProperty *property : m_properties)
    {
        if (!property->seedIntoStore())
        {
            m_logger->errorStream()
                << "Failed to seed property '" << property->getKey() << "' into the store";
            return false;
        }
    }

    return true;
}

void AbsPropertyManager::registerProperty(AbsRegisteredProperty &property)
{
    m_properties.push_back(&property);
}

act::foundation::AbsLogger &AbsPropertyManager::accessLogger() const
{
    return *m_logger;
}

} // namespace act::property
