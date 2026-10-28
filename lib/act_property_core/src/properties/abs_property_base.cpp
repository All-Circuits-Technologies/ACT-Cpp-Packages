// SPDX-FileCopyrightText: 2026 Benoit Rolandeau <benoit.rolandeau@allcircuits.com>
//
// SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

#include "act_property_core/properties/abs_property_base.hpp"

#include "act_property_core/properties/abs_property_registry.hpp"

#include <string>
#include <utility>

namespace act::property
{

AbsPropertyBase::AbsPropertyBase(AbsPropertyRegistry &registry, std::string key)
    : m_registry(registry),
      m_key(std::move(key))
{
    m_registry.registerProperty(*this);
}

AbsPropertyBase::~AbsPropertyBase()
{
    m_registry.unregisterProperty(*this);
}

AbsPropertyStore &AbsPropertyBase::accessStore() const
{
    return m_registry.accessStore();
}

act::foundation::AbsLogger &AbsPropertyBase::accessLogger() const
{
    return m_registry.accessLogger();
}

} // namespace act::property
