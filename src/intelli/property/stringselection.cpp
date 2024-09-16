/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/property/stringselection.h"

using namespace intelli;

QString const StringSelectionProperty::S_INVALID = QStringLiteral("N/A");

StringSelectionProperty::StringSelectionProperty(QString const& ident,
                                                         QString const& name,
                                                         QString const& brief,
                                                         QStringList const& allowedValues,
                                                         QString const& _default) :
    m_values(allowedValues)
{
    setObjectName(name);
    m_id = ident;
    m_brief = brief;
    m_value = _default;

    validate();

    m_initValue = m_value;
}

void
StringSelectionProperty::setValues(QStringList const& values)
{
    m_values = values;
    validate();
}

QStringList const&
StringSelectionProperty::values() const
{
    return m_values;
}

QString const&
StringSelectionProperty::selectedValue() const
{
    return m_value;
}

bool
StringSelectionProperty::select(const QString& value)
{
    return select(indexOf(value));
}

bool
StringSelectionProperty::select(int index)
{
    if (index < 0 || index >= m_values.size())
    {
        return false;
    }

    m_value = m_values.at(index);
    emit changed();
    return true;
}

int
StringSelectionProperty::indexOf(const QString& value)
{
    return m_values.indexOf(value);
}

QVariant
StringSelectionProperty::valueToVariant(QString const& unit,
                                            bool* success) const
{
    return gt::valueSuccess(m_value, success);
}

bool
StringSelectionProperty::setValueFromVariant(QVariant const& val,
                                                 QString const& unit)
{
    m_value = val.toString();
    validate();
    return true;
}

void
StringSelectionProperty::validate()
{
    if (m_value.isEmpty() || !m_values.contains(m_value))
    {
        m_value = (m_values.isEmpty()) ? S_INVALID : m_values.front();
    }
}

gt::PropertyFactoryFunction
intelli::makeStringSelectionProperty(QStringList allowedValues)
{
    return [vals = std::move(allowedValues)](QString const& id) -> GtAbstractProperty*{
        return new StringSelectionProperty(id, id, vals);
    };
}
