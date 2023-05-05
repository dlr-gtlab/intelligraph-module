/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_igcolorporperty.h"

GtIgColorPorperty::GtIgColorPorperty(QString const& ident,
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
GtIgColorPorperty::valueToVariant(const QString& unit, bool* ok) const
{
    return gt::valueSuccess(QVariant::fromValue(m_value.name()), ok);
}

bool
GtIgColorPorperty::setValueFromVariant(const QVariant& val, const QString& unit)
{
    QString name = val.toString();

    if (name.isEmpty()) return false;

    m_value = name;

    return true;
}

GtIgColorPorperty&
GtIgColorPorperty::operator=(const QColor& val)
{
    setVal(val);
    emit changed();
    return *this;
}
