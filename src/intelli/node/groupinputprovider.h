/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_GROUPINPUTPROVIDER_H
#define GT_INTELLI_GROUPINPUTPROVIDER_H

#include <intelli/node/abstractgroupprovider.h>

namespace intelli
{

class GroupInputProvider : public AbstractGroupProvider<PortType::In>
{
    Q_OBJECT

public:

    Q_INVOKABLE GroupInputProvider();

protected:

    NodeDataPtr eval(PortId outId) override;
};

} // namespace intelli

#endif // GT_INTELLI_GROUPINPUTPROVIDER_H
