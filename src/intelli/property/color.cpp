/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
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
