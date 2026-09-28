/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GROUPINPUTPROVIDER_H
#define GT_INTELLI_GROUPINPUTPROVIDER_H

#include <intelli/exports.h>
#include <intelli/node/abstractgroupprovider.h>

namespace intelli
{

class GT_INTELLI_EXPORT GroupInputProvider : public AbstractGroupProvider<PortType::In>
{
    Q_OBJECT

public:

    Q_INVOKABLE GroupInputProvider();

protected:

    void eval() override;
};

} // namespace intelli

#endif // GT_INTELLI_GROUPINPUTPROVIDER_H
