/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GROUPOUTPUTPROVIDER_H
#define GT_INTELLI_GROUPOUTPUTPROVIDER_H

#include <intelli/node/abstractgroupprovider.h>

namespace intelli
{

class GroupOutputProvider : public AbstractGroupProvider<PortType::Out>
{
    Q_OBJECT

public:

    Q_INVOKABLE GroupOutputProvider();

protected:

    void eval() override;
};

} // namespace intelli

#endif // GT_INTELLI_GROUPOUTPUTPROVIDER_H
