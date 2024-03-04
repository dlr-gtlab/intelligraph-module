/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 20.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef LOGICDISPLAYNODE_H
#define LOGICDISPLAYNODE_H

#include <intelli/node.h>

namespace intelli
{

class LogicDisplayNode : public intelli::Node
{
    Q_OBJECT

public:

    Q_INVOKABLE LogicDisplayNode();

private:

    PortId m_in;
};

}

#endif // LOGICDISPLAYNODE_H
