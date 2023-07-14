/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 27.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_igstringselectionproperty.h"

QString const GtIgStringSelectionProperty::S_INVALID = QStringLiteral("N/A");

GtIgStringSelectionProperty::GtIgStringSelectionProperty(QString const& ident,
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
GtIgStringSelectionProperty::setValues(QStringList const& values)
{
    m_values = values;
    validate();
}

QStringList const&
GtIgStringSelectionProperty::values() const
{
    return m_values;
}

QString const&
GtIgStringSelectionProperty::selectedValue() const
{
    return m_value;
}

bool
GtIgStringSelectionProperty::select(const QString& value)
{
    return select(indexOf(value));
}

bool
GtIgStringSelectionProperty::select(int index)
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
GtIgStringSelectionProperty::indexOf(const QString& value)
{
    return m_values.indexOf(value);
}

QVariant
GtIgStringSelectionProperty::valueToVariant(QString const& unit,
                                            bool* success) const
{
    return gt::valueSuccess(m_value, success);
}

bool
GtIgStringSelectionProperty::setValueFromVariant(QVariant const& val,
                                                 QString const& unit)
{
    m_value = val.toString();
    validate();
    return true;
}

void
GtIgStringSelectionProperty::validate()
{
    if (m_value.isEmpty() || !m_values.contains(m_value))
    {
        m_value = (m_values.isEmpty()) ? S_INVALID : m_values.front();
    }
}

gt::PropertyFactoryFunction
gt::ig::makeStringSelectionProperty(QStringList allowedValues)
{
    return [vals = std::move(allowedValues)](QString const& id) -> GtAbstractProperty*{
        return new GtIgStringSelectionProperty(id, id, vals);
    };
}
