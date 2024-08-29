/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/property/color.h"

using namespace intelli;

ColorPorperty::ColorPorperty(QString const& ident,
                                     QString const& name,
                                     QString const& brief,
                                     QColor const& color)
{
    m_id = ident;
    setObjectName(name);
    m_brief = brief;
    m_initValue = color;
    m_value = color;
}

QVariant
ColorPorperty::valueToVariant(const QString& unit, bool* ok) const
{
    return gt::valueSuccess(QVariant::fromValue(m_value.name()), ok);
}

bool
ColorPorperty::setValueFromVariant(const QVariant& val, const QString& unit)
{
    QString name = val.toString();

    if (name.isEmpty()) return false;

    m_value = name;

    return true;
}

ColorPorperty&
ColorPorperty::operator=(const QColor& val)
{
    setVal(val);
    emit changed();
    return *this;
}
