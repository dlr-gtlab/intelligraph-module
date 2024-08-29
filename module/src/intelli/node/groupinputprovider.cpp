/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/node/groupinputprovider.h"

using namespace intelli;

GroupInputProvider::GroupInputProvider() :
    AbstractGroupProvider("Input Provider")
{
    setPos({-250, 0});
}

void
GroupInputProvider::eval()
{
    // forward input data to output
    for (auto const& p : ports(PortType::Out))
    {
        setNodeData(p.id(), nodeData(p.id()));
    }
}
