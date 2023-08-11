/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 24.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/exec/executor.h"

#include "intelli/node.h"
#include "intelli/private/node_impl.h"

using namespace intelli;

Executor::Executor() = default;

bool
Executor::isReady() const
{
    return true;
}

bool
Executor::canEvaluateNode(Node& node, PortIndex outIdx)
{
    auto* p = node.pimpl.get();

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
            if (outIdx != invalid<PortIndex>())
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
Executor::doEvaluate(Node& node, PortIndex idx)
{
    auto& p = *node.pimpl;

    gtDebug().verbose().nospace()
        << "### Evaluating node:  '" << node.objectName()
        << "' at output idx '" << idx << "'";

    assert(idx < p.outData.size());

    auto& out = p.outData.at(idx);

    out = node.eval(p.outPorts.at(idx).id());

    return out != nullptr;
}

void
Executor::doEvaluateAndDiscard(Node& node)
{
    gtDebug().verbose().nospace()
        << "### Evaluating node:  '" << node.objectName() << "'";

    node.eval(invalid<PortId>());
}

NodeImpl&
Executor::accessImpl(Node& node)
{
    return *node.pimpl;
}
