/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.9.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef UINTPROPERTY_H
#define UINTPROPERTY_H

#include <intelli/exports.h>

#include <gt_intproperty.h>

namespace intelli
{

class GT_INTELLI_EXPORT UIntProperty : public GtProperty<unsigned>
{
    Q_OBJECT

    Q_PROPERTY(int get READ getVal WRITE setVal)

public:

    using GtProperty<unsigned>::operator=;

    Q_INVOKABLE UIntProperty(QString const& ident,
                             QString const& name,
                             QString const& brief,
                             unsigned value = 0);

    Q_INVOKABLE UIntProperty(QString const& ident,
                             QString const& name,
                             unsigned value = 0);

    /**
     * @brief valueToVariant
     * @return
     */
    QVariant valueToVariant(QString const& unit,
                            bool* success = nullptr) const override;

    /**
     * @brief setValueFromVariant
     * @param val
     * @return
     */
    GT_NO_DISCARD
    bool setValueFromVariant(QVariant const& val,
                             QString const& unit) override;
};

/**
 * @brief Creates a property factory for ints with a default value
 */
GT_INTELLI_EXPORT
gt::PropertyFactoryFunction makeUIntProperty(int value);

} // namespace intelli

#endif // UINTPROPERTY_H
