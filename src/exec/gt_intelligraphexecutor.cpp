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
GtIntellIGraphExecutor::isReady() const
{
    return true;
}

bool
GtIntellIGraphExecutor::canEvaluateNode(GtIntelliGraphNode& node, PortIndex outIdx)
{
    auto* p = node.pimpl.get();

//    if (!p->active)
//    {
//        gtWarning().verbose()
//            << tr("Node is not active!")
//            << gt::brackets(node.objectPath());
//        return false;
//    }

//    if (p->state == GtIntelliGraphNode::Evaluating)
//    {
//        gtWarning().medium()
//            << tr("Node already evaluating!")
//            << gt::brackets(node.objectPath());
//        return false;
//    }

    PortIndex inIdx{0};
    for (auto const& data : p->inData)
    {
        auto const& port = p->inPorts.at(inIdx++);

        // check if data is required and valid
        if (!port.optional && !data)
        {
            gtWarning().verbose()
                << tr("Node is not ready for evaluation!")
                << gt::brackets(node.objectPath());

            // emit invalidated signals
            if (outIdx != gt::ig::invalid<PortIndex>())
            {
                emit node.outDataInvalidated(outIdx);
                return false;
            }

            for (PortIndex outIdx{0}; outIdx < p->outPorts.size(); ++outIdx)
            {
                emit node.outDataInvalidated(outIdx++);
            }
            return false;
        };
    }

    return true;
}

bool
GtIntellIGraphExecutor::doEvaluate(GtIntelliGraphNode& node, PortIndex idx)
{
    auto& p = *node.pimpl;

    gtDebug().verbose().nospace()
        << "### Evaluating node:  '" << node.objectName()
        << "' at output idx '" << idx << "'";

    assert(idx < p.outData.size());

    auto& out = p.outData.at(idx);

    out = node.eval(p.outPorts.at(idx).id());

    emit node.evaluated(idx);

    return out != nullptr;
}

void
GtIntellIGraphExecutor::doEvaluateAndDiscard(GtIntelliGraphNode& node)
{
    gtDebug().verbose().nospace()
        << "### Evaluating node:  '" << node.objectName() << "'";

    node.eval(gt::ig::invalid<PortId>());

    emit node.evaluated();
}

GtIntelliGraphNodeImpl&
GtIntellIGraphExecutor::accessImpl(GtIntelliGraphNode& node)
{
    return *node.pimpl;
}
