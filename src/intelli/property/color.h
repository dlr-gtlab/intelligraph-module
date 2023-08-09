/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_COLORPROPERTY_H
#define GT_INTELLI_COLORPROPERTY_H

#include <intelli/exports.h>

#include <gt_property.h>

#include <QColor>

namespace intelli
{

class GT_INTELLI_EXPORT ColorPorperty : public GtProperty<QColor>
{
    Q_OBJECT

public:

    using GtProperty<QColor>::operator();

    ColorPorperty(QString const& ident,
                      QString const& name,
                      QString const& brief,
                      QColor const& color = {});


    /**
     * @brief Overloaded function to convert internal property value to
     * QVariant. Value is converted based on given unit.
     * @param unit Unit into which the value is to be converted.
     * @param ok Whether conversion was successfull or not.
     * @return Value as QVariant
     */
    QVariant valueToVariant(QString const& unit, bool* ok = nullptr) const override;

    /**
     * @brief Overloaded function to set internal property value from given
     * QVariant. Value of QVariant is converted based on given unit.
     * @param val New value in form of QVariant.
     * @param unit Unit of the input value.
     * @param success Whether the value could be set or not
     * @return
     */
    GT_NO_DISCARD
    bool setValueFromVariant(QVariant const& val, QString const& unit) override;

    ColorPorperty& operator=(const QColor& val);
};

} // namespace intelli

#endif // GT_INTELLI_COLORPROPERTY_H
