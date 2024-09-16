/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_STRINGSELECTIONPROPERTY_H
#define GT_INTELLI_STRINGSELECTIONPROPERTY_H

#include <intelli/exports.h>

#include <gt_stringproperty.h>

namespace intelli
{

/**
 * @brief The GtIgStringSelectionProperty class.
 * Property that allows the user to select an entry
 */
class GT_INTELLI_EXPORT StringSelectionProperty : public GtProperty<QString>
{
    Q_OBJECT

public:

    static QString const S_INVALID;

    /**
     * @brief constructor
     * @param ident Identification
     * @param name Display text of property
     * @param brief Brief of property
     * @param allowedValues Allowed values for selection
     */
    StringSelectionProperty(QString const& ident,
                                QString const& name,
                                QString const& brief,
                                QStringList const& allowedValues,
                                QString const& _default = S_INVALID);

    /**
     * @brief constructor
     * @param ident Identification
     * @param name Display text and brief of property
     * @param allowedValues Allowed values for selection
     */
    StringSelectionProperty(QString const& ident,
                                QString const& name,
                                QStringList const& allowedValues,
                                QString _default = S_INVALID) :
        StringSelectionProperty(ident, name, name, allowedValues, _default)
    {}

    /**
     * @brief Sets the allowed values that are used for selection. Updates the
     * selected text if its not vali anymore
     * @param values New values
     */
    void setValues(QStringList const& values);

    /**
     * @brief Returns the internal list of possible values
     * @return Values
     */
    QStringList const& values() const;

    /**
     * @brief Returns the currently selected value
     * @return selected value
     */
    QString const& selectedValue() const;

    /**
     * @brief Selects the value matched by the parameter "value"
     * @param value Value to match
     * @return Success
     */
    bool select(QString const& value);

    /**
     * @brief Selects the index
     * @param index Index to select
     * @return success
     */
    bool select(int index);

    /**
     * @brief Returns the index of value or -1 if it fails
     * @param value Value
     * @return Index (-1 if it does not exist in the list)
     */
    int indexOf(QString const& value);

    GT_NO_DISCARD QVariant valueToVariant(const QString& unit,
                                          bool* success = 0) const override;

    /**
     * @brief setValueFromVariant
     * @param val
     * @param unit
     * @param success
     * @return
     */
    GT_NO_DISCARD bool setValueFromVariant(const QVariant& val,
                                           const QString& unit) override;

    /**
     * @brief Validates the currently selected value. May change the selected
     * value if its invalid.
     */
    void validate();

protected:

    QStringList m_values;
};

GT_INTELLI_EXPORT
gt::PropertyFactoryFunction
makeStringSelectionProperty(QStringList allowedValues);

} // namespace intelli

#endif // GT_INTELLI_STRINGSELECTIONPROPERTY_H
