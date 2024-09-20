/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_UINTPROPERTY_H
#define GT_INTELLI_UINTPROPERTY_H

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

#endif // GT_INTELLI_UINTPROPERTY_H
