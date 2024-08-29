/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 12.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef TESTNODE_H
#define TESTNODE_H

#include <intelli/node.h>

class TestNode : public intelli::Node
{
    Q_OBJECT

public:

    bool failEvaluation = false;

    static void registerOnce();

    Q_INVOKABLE TestNode();

    using Node::insertInPort;
    using Node::insertOutPort;
    using Node::addInPort;
    using Node::addOutPort;
    using Node::removePort;

protected:

    bool handleNodeEvaluation(intelli::NodeDataInterface& model) override;

};

#endif // TESTNODE_H