/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/node/doubletobool.h"
#include "intelli/data/bool.h"
#include "intelli/data/double.h"

#include "intelli/nodefactory.h"

using namespace intelli;

#ifdef GTIG_DEVELOPER_PREVIEW
GT_INTELLI_REGISTER_NODE(CheckDoubleNode, "Conditional")
#endif

CheckDoubleNode::CheckDoubleNode() :
    Node("Is smaller than")
{
    m_inA = addInPort(typeId<DoubleData>());
    m_inB = addInPort(typeId<DoubleData>());

    m_out = addOutPort(typeId<BoolData>());
}

Node::NodeDataPtr
CheckDoubleNode::eval(PortId outId)
{
    if (outId != m_out) return {};

    double a = 0.0, b = 0.0;

    if (auto* inA = nodeData<DoubleData*>(m_inA)) a = inA->value();
    if (auto* inB = nodeData<DoubleData*>(m_inB)) b = inB->value();

    gtInfo() << "EVALUATION (BOOL NODE):" << a << "<" << b << (a < b);

    return std::make_shared<BoolData>(a < b);
}

