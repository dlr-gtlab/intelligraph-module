
#ifndef GT_INTELLI_METAENUMPROPERTY_H
#define GT_INTELLI_METAENUMPROPERTY_H

#include <intelli/exports.h>

#include "gt_modeproperty.h"
#include "gt_modetypeproperty.h"

#include <QMetaEnum>

namespace intelli
{

/**
 * @brief The MetaEnumProperty class.
 *
 * Works similar to GtEnumProperty but does not require dependency to enum type
 * when defining and instantiating. Can be used to hide dependencies to GUI
 * specific enums in batch mode.
 */
class GT_INTELLI_EXPORT MetaEnumProperty : public GtModeProperty
{
    // omitting Q_OBJECT macro here, otherwise an incorrect property editor is
    // used. Cannot use "GtPropertyModeItem" as it is not exported currently

public:

    explicit MetaEnumProperty(QString const& ident,
                              QString const& name,
                              QString const& brief = {}) :
        GtModeProperty(ident, name, brief)
    { }

    /**
     * @brief Reigsters the enum property. One can only register a enum once and
     * this type should not change.
     * @param metaEnum Enum type to register
     */
    bool registerEnum(QMetaEnum metaEnum);

    /**
     * @brief Reigsters the enum property based on the template argument. One
     * can only register a enum once and this type should not change.
     * @tparam T Enum type to register
     */
    template <typename T>
    bool registerEnum()
    {
        static_assert(std::is_enum<T>::value, "expected enum type");

        QMetaEnum metaEnum = QMetaEnum::fromType<T>();
        return registerEnum(metaEnum);
    }

    /**
     * @brief Returns whether this property was initialized with a
     * @return Whether the property is initialized
     */
    bool isInitialized() const;

    /**
     * @brief Getter for the current enum value.
     * @return Enum which is part of this property.
     */
    QMetaEnum const& getMetaEnum() const { return m_enum; }

    /**
     * @brief Returns the currently stored enum type as type `T`. The property
     * must be initialized.
     * @return Value as type `T`.
     */
    template <typename T>
    T getEnum() const
    {
        static_assert(std::is_enum<T>::value, "expected enum type");

        if (!isInitialized() || QMetaEnum::fromType<T>().name() != m_enum.name())
        {
            gtErrorId("IntelliGraph")
                << __FUNCTION__
                << tr("failed, property '%1' is unitialized/invalid!")
                       .arg(ident());
            return T{};
        }

        QString const& key = GtModeProperty::getVal();
        return static_cast<T>(getMetaEnum().keyToValue(key.toUtf8()));
    }

    /**
     * @brief Sets the enum value to store. The property must be initialized.
     * @param Value as type `T`.
     * @return success
     */
    template<typename T>
    inline bool setEnum(T value)
    {
        static_assert(std::is_enum<T>::value, "expected enum type");

        if (!isInitialized() || QMetaEnum::fromType<T>().name() != m_enum.name())
        {
            gtErrorId("IntelliGraph")
                << __FUNCTION__
                << tr("failed, property '%1' is unitialized/invalid!")
                       .arg(ident());
            return false;
        }

        bool success = true;
        GtModeProperty::setVal(getMetaEnum().valueToKey(static_cast<int>(value)), &success);
        return success;
    }

    GT_NO_DISCARD
    bool setValueFromVariant(QVariant const& val,
                             QString const& unit) override;

protected:

    bool validateValue(QString const& value) override;

private:

    QMetaEnum m_enum;
};

} // namespace intelli

#endif // GT_INTELLI_METAENUMPROPERTY_H
