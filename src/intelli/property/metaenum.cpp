
#include "metaenum.h"

#include "gt_modetypeproperty.h"

using namespace intelli;

bool
MetaEnumProperty::registerEnum(QMetaEnum metaEnum)
{
    if (isInitialized())
    {
        // already initialized
        if (m_enum.isValid() && m_enum.name() == metaEnum.name()) return true;

        gtError() << tr("MetaEnumProperty (%1) is already initialized! "
                        "Current enum type '%2' vs. new enum type '%3'")
                         .arg(ident(), metaEnum.name(), m_enum.name());
        return false;
    }

    m_enum = metaEnum;
    /// adapted from GtEnumProperty
    for (int i = 0; i < m_enum.keyCount(); i++)
    {
        auto* subProperty = new GtModeTypeProperty(m_enum.key(i), "");

        subProperty->setParent(this);
        registerSubProperty(*subProperty);
    }
    return true;
}

bool
MetaEnumProperty::isInitialized() const
{
    return getMetaEnum().isValid();
}

bool
MetaEnumProperty::setValueFromVariant(QVariant const& value,
                                      QString const& unit)
{
    if (!isInitialized())
    {
        return GtModeProperty::setValueFromVariant(value, unit);
    }

    /// adapted from GtEnumProperty
    bool canConvert = false;
    getMetaEnum().keyToValue(value.toString().toUtf8(), &canConvert);

    return canConvert && GtModeProperty::setValueFromVariant(value, unit);
}

bool
MetaEnumProperty::validateValue(QString const& value)
{
    // accept any type if the property is not yet initialized
    if (!isInitialized()) return true;

    // GtModeProperty::validateValue is private
    return modes().contains(value);
}
