/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.9.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
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
    m_value = val.toUInt(&ok);
    return ok;
}

gt::PropertyFactoryFunction
intelli::makeUIntProperty(int value)
{
    return [=](QString const& id){
        return new UIntProperty(id, id, value);
    };
}
