/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 24.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphexecutor.h"

#include "gt_intelligraphnode.h"
#include "private/intelligraphnode_impl.h"

using namespace gt::ig;

GtIntellIGraphExecutor::GtIntellIGraphExecutor() = default;


bool
GtIntellIGraphExecutor::canEvaluateNode(GtIntelliGraphNode& node, PortIndex outIdx)
{
    auto* p = node.pimpl.get();

    if (!p->active)
    {
        gtWarning().verbose()
            << QObject::tr("Node is not active!")
            << gt::brackets(node.objectName());
        return false;
    }

    if (p->state == GtIntelliGraphNode::Evaluating)
    {
        gtWarning().medium()
            << QObject::tr("Node already evaluating!")
            << gt::brackets(node.objectName());
        return false;
    }

    PortIndex inIdx{0};
    for (auto const& data : p->inData)
    {
        auto const& port = p->inPorts.at(inIdx++);

        // check if data is required and valid
        if (!port.optional && !data)
        {
            gtWarning().verbose()
                << QObject::tr("Node is not ready for evaluation!")
                << gt::brackets(node.objectName());

            // emit invalidated signals
            if (outIdx != gt::ig::invalid<PortIndex>())
            {
                emit node.outDataInvalidated(outIdx);
                return false;
            }

            // update all ports
            outIdx = PortIndex{0};
            for (auto const& _ : p->outPorts)
            {
                emit node.outDataInvalidated(outIdx++);
            }
            return false;
        };
    }

    return true;
}

void
GtIntellIGraphExecutor::evaluateNode(GtIntelliGraphNode& node)
{
    auto* p = node.pimpl.get();

    if (!canEvaluateNode(node)) return;

    p->state = GtIntelliGraphNode::Evaluating;
    auto resetState = gt::finally([p](){
        p->state = GtIntelliGraphNode::Evaluated;
    });

    // trigger eval if no outport exists
    if (p->outPorts.empty() && !p->inPorts.empty())
    {
        gtDebug().verbose().nospace()
            << "### Evaluating node:  '" << node.objectName() << "'";

        node.eval(gt::ig::invalid<PortId>());
        return;
    }

    // iterate over all output ports
    PortIndex idx{0};
    for (auto const& port : p->outPorts)
    {
        auto id = port.id();

        gtDebug().verbose().nospace()
            << "### Evaluating node:  '" << node.objectName()
            << "' at output idx '" << idx << "'";

        auto& out = p->outData.at(idx);

        out = node.eval(id);

        out ? emit node.outDataUpdated(idx) : emit node.outDataInvalidated(idx);

        idx++;
    }
}

void
GtIntellIGraphExecutor::evaluatePort(GtIntelliGraphNode& node, PortIndex idx)
{
    auto* p = node.pimpl.get();

    if (idx >= p->outPorts.size()) return;

    if (!canEvaluateNode(node, idx)) return;

    gtDebug().verbose().nospace()
        << "### Evaluating node:  '" << node.objectName()
        << "' at output idx '" << idx << "'";

    p->state = GtIntelliGraphNode::Evaluating;
    auto resetState = gt::finally([p](){
        p->state = GtIntelliGraphNode::Evaluated;
    });

    auto& out = p->outData.at(idx);

    out = node.eval(p->outPorts.at(idx).id());

    out ? emit node.outDataUpdated(idx) : emit node.outDataInvalidated(idx);
}

