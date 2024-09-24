/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/property/uint.h"

using namespace intelli;

UIntProperty::UIntProperty(QString const& ident,
                           QString const& name,
                           unsigned int value) :
    UIntProperty(ident, name, name, value)
{ }

UIntProperty::UIntProperty(QString const& ident,
                           QString const& name,
                           QString const& brief,
                           unsigned int value)
{
    setObjectName(name);
    m_id = ident;
    m_value = value;
    m_brief = brief;
    m_initValue = value;
}

QVariant
UIntProperty::valueToVariant(QString const& unit, bool* success) const
{
    return gt::valueSuccess(QVariant::fromValue(m_value), success);
}

bool
UIntProperty::setValueFromVariant(QVariant const& val, QString const& unit)
{
    bool ok = true;
    setVal(val.toUInt(&ok));
    return ok;
}

gt::PropertyFactoryFunction
intelli::makeUIntProperty(int value)
{
    return [=](QString const& id){
        return new UIntProperty(id, id, value);
    };
}
