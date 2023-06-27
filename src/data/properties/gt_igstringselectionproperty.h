/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 27.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTIGSTRINGSELECTIONPROPERTY_H
#define GTIGSTRINGSELECTIONPROPERTY_H

#include "gt_intelligraph_exports.h"
#include "gt_stringproperty.h"

/**
 * @brief The GtIgStringSelectionProperty class.
 * Property that allows the user to select an entry
 */
class GT_IG_EXPORT GtIgStringSelectionProperty : public GtProperty<QString>
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
    GtIgStringSelectionProperty(QString const& ident,
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
    GtIgStringSelectionProperty(QString const& ident,
                                QString const& name,
                                QStringList const& allowedValues,
                                QString _default = S_INVALID) :
        GtIgStringSelectionProperty(ident, name, name, allowedValues, _default)
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

namespace gt
{
namespace ig
{

GT_IG_EXPORT
gt::PropertyFactoryFunction
makeStringSelectionProperty(QStringList allowedValues);

} // namespace ig

} // namespace gt

#endif // GTIGSTRINGSELECTIONPROPERTY_H
