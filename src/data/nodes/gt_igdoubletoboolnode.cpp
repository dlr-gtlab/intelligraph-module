/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_igdoubletoboolnode.h"
#include "gt_igbooldata.h"
#include "gt_igdoubledata.h"

#include "gt_intelligraphnodefactory.h"

#ifdef GTIG_DEVELOPER_PREVIEW
GTIG_REGISTER_NODE(GtIgCheckDoubleNode, "Conditional")
#endif

GtIgCheckDoubleNode::GtIgCheckDoubleNode() :
    GtIntelliGraphNode("Is smaller than")
{
    m_inA = addInPort(gt::ig::typeId<GtIgDoubleData>());
    m_inB = addInPort(gt::ig::typeId<GtIgDoubleData>());

    m_out = addOutPort(gt::ig::typeId<GtIgBoolData>());
}

GtIntelliGraphNode::NodeData
GtIgCheckDoubleNode::eval(PortId outId)
{
    if (outId != m_out) return {};

    double a = 0.0, b = 0.0;

    if (auto* inA = nodeData<GtIgDoubleData*>(m_inA)) a = inA->value();
    if (auto* inB = nodeData<GtIgDoubleData*>(m_inB)) b = inB->value();

    gtInfo() << "EVALUATION (BOOL NODE):" << a << "<" << b << (a < b);

    return std::make_shared<GtIgBoolData>(a < b);
}

