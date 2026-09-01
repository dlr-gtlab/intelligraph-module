/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#include "intelli/node/control/accumulator.h"

#include "intelli/graphdatamodel.h"
#include "intelli/graphexecutor.h"
#include "intelli/data/stringlist.h"
#include "intelli/data/string.h"

#include "intelli/private/utils.h"

#include <gt_utilities.h>
#include <gt_eventloop.h>

using namespace intelli;

const char* C_NAME_IN_NODE = "Input";
const char* C_NAME_OUT_NODE = "Output";
const char* C_NAME_LAST_ITER_NODE = "Last Iteration";

AccumulatorGraphNode::AccumulatorGraphNode() :
    Graph(QStringLiteral("Accumulator"), false)
{
    NodeId nextId{0};

    Position offset{0, 100};

    m_listIn = addInPort(makePort(typeId<StringListData>()));
    m_out = addOutPort(makePort(typeId<StringData>()));

    auto input = std::make_unique<GraphInputProvider>();
    input->setCaption(C_NAME_IN_NODE);
    input->setPos(input->pos() - offset);
    input->setDefault(true);
    input->setId(nextId++);
//    synchronizePorts(*input);
    auto in = appendNode(std::move(input), NodeIdPolicy::Keep);

    auto output = std::make_unique<GraphOutputProvider>();
    output->setCaption(C_NAME_OUT_NODE);
    output->setPos(output->pos());
    output->setDefault(true);
    output->setId(nextId++);
//    synchronizePorts(*output);
    auto out = appendNode(std::move(output), NodeIdPolicy::Keep);

    auto lastIter = std::make_unique<LastIterationProvider>();
    lastIter->setCaption(C_NAME_LAST_ITER_NODE);
    lastIter->setPos(lastIter->pos() + offset);
    lastIter->setDefault(true);
    lastIter->setId(nextId++);
    synchronizePorts(*out, *lastIter);
    appendNode(std::move(lastIter), NodeIdPolicy::Keep);

    in->addPort(PortInfo::customId(m_listIn, typeId<StringData>()));
    out->addPort(PortInfo::customId(m_out, typeId<StringData>()));

    setNodeEvalMode(NodeEvalMode::Blocking);
}

void
AccumulatorGraphNode::eval()
{
    auto makeError = [this](){
        return gt::quoted(relativeNodePath(*this), "[", "] ") +
               tr("evaluation failed!");
    };

    if (!port(m_out))
    {
        gtError() << makeError() << tr("invalid output port!");
        return evalFailed();
    }

    auto* dataModel = qobject_cast<GraphDataModel*>(exec::nodeDataInterface(*this));
    if (!dataModel)
    {
        gtError() << makeError() << tr("data model not found!");
        return evalFailed();
    }

    if (!setNodeData(m_out, nullptr))
    {
        gtError() << makeError();
        return evalFailed();
    }

    // setup
    auto listData = nodeData<StringListData>(m_listIn);
    if (!listData || !port(m_listIn))
    {
        gtError() << makeError() << tr("invalid list data!");
        return evalFailed();
    }

    QStringList list = listData->value();

    auto* inputNode = findDirectChild<GraphInputProvider*>(C_NAME_IN_NODE);
    auto* lastIterNode = findDirectChild<GraphInputProvider*>(C_NAME_LAST_ITER_NODE);
    auto* outputNode = findDirectChild<GraphOutputProvider*>(C_NAME_OUT_NODE);

    if (!inputNode || !outputNode || !outputNode)
    {
        gtError() << makeError() << tr("input/ouput providers not found!");
        return evalFailed();
    }

    if (!dataModel->setNodeData(lastIterNode->uuid(), m_out, makeNodeData<StringData>()))
    {
        gtError() << makeError()
                  << tr("failed to reset last iter data for port '%1'!");
        return evalFailed();
    }

    for (QString const& current : list)
    {
        // set input data
        for (NodePort const& port : { *port(m_listIn) })
        {
            if (!inputNode->port(port.id()))
            {
                gtError() << makeError()
                          << tr("port '%1' in input provider not found!")
                                 .arg(toString(port));
                return evalFailed();
            }

            if (!dataModel->setNodeData(inputNode->uuid(), port.id(), makeNodeData<StringData>(current)))
            {
                gtError() << makeError()
                          << tr("failed to set input data for port '%1'!")
                                 .arg(toString(port));
                return evalFailed();
            }
        }

        GtEventLoop loop{std::chrono::seconds{2}};

        // evaluate branch
        GraphExecutor executor{*this, *dataModel};

        loop.connectSuccess(&executor, &GraphExecutor::targetNodesEvaluated);
        loop.connectAbort(this, &Graph::graphAboutToBeDeleted);

        auto future = executor.evaluateNode(outputNode->id());

        // TODO: cannot block main thread here!
        auto status = loop.exec();
        if (status != GtEventLoop::Success)
        {
            return evalFailed();
        }

        // set output data
        for (NodePort const& port : { *port(m_out) })
        {
            if (!lastIterNode->port(port.id()))
            {
                gtError() << makeError()
                          << tr("port '%1' in last iter provider not found!")
                                 .arg(toString(port));
                return evalFailed();
            }

            if (!dataModel->setNodeData(lastIterNode->uuid(), port.id(), dataModel->nodeData(outputNode->uuid(), port.id())))
            {
                gtError() << makeError()
                          << tr("failed to set last iter data for port '%1'!")
                                 .arg(toString(port));
                return evalFailed();
            }
        }
    }

    auto const& port = *this->port(m_out);

    if (!outputNode->port(port.id()))
    {
        gtError() << makeError()
                  << tr("port '%1' in output provider not found!")
                         .arg(toString(port));
        return evalFailed();
    }

    if (!setNodeData(port.id(), dataModel->nodeData(lastIterNode->uuid(), port.id())))
    {
        gtError() << makeError()
                  << tr("failed to set output data for port '%1'!")
                         .arg(toString(port));
        return evalFailed();
    }
}

