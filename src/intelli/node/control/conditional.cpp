/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#include "intelli/node/control/conditional.h"

#include "intelli/data/bool.h"
#include "intelli/data/double.h"

#include "intelli/nodedatainterface.h"
#include "intelli/graphdatamodel.h"
#include "intelli/graphexecutor.h"

#include "intelli/private/utils.h"

#include <gt_eventloop.h>


using namespace intelli;

const char* C_NAME_IF_IN_NODE = "If Input";
const char* C_NAME_ELSE_IN_NODE = "Else Input";
const char* C_NAME_IF_OUT_NODE = "If Output";
const char* C_NAME_ELSE_OUT_NODE = "Else Output";

ConditionalGroupNode::ConditionalGroupNode() :
    Graph(QStringLiteral("Conditional"), false)
{
    // add static port first
    m_condition = addStaticInPort(
        PortInfo::customId(PortId{0}, typeId<BoolData>())
            .setCaption("condition")
            .setOptional(false));

    NodeId nextId{0};

    for (auto type : { ConditionalBranchType::IfBranch, ConditionalBranchType::ElseBranch })
    {
        Position offset{0, 100};
        if (type == ConditionalBranchType::IfBranch) offset.ry() *= -1;

        auto input = std::make_unique<ConditionalInputProvider>();
        input->setCaption(type == ConditionalBranchType::IfBranch ?
                              C_NAME_IF_IN_NODE : C_NAME_ELSE_IN_NODE);
        input->setPos(input->pos() + offset);
        input->setDefault(true);
        input->setId(nextId++);
        synchronizePorts(*input);
        appendNode(std::move(input), NodeIdPolicy::Keep);

        auto output = std::make_unique<ConditionalOutputProvider>();
        output->setCaption(type == ConditionalBranchType::IfBranch ?
                               C_NAME_IF_OUT_NODE : C_NAME_ELSE_OUT_NODE);
        output->setPos(output->pos() + offset);
        output->setDefault(true);
        output->setId(nextId++);
        synchronizePorts(*output);
        appendNode(std::move(output), NodeIdPolicy::Keep);
    }

    addInPort(makePort(typeId<DoubleData>()));
    addOutPort(makePort(typeId<DoubleData>()));

    setNodeEvalMode(NodeEvalMode::Blocking);
}

ConditionalInputProvider*
ConditionalGroupNode::inputProvider(ConditionalBranchType type)
{
    return findDirectChild<ConditionalInputProvider*>(
        (type == ConditionalBranchType::IfBranch) ? C_NAME_IF_IN_NODE : C_NAME_ELSE_IN_NODE);
}

ConditionalInputProvider const*
ConditionalGroupNode::inputProvider(ConditionalBranchType type) const
{
    return const_cast<ConditionalGroupNode*>(this)->inputProvider(type);
}

ConditionalOutputProvider*
ConditionalGroupNode::outputProvider(ConditionalBranchType type)
{
    return findDirectChild<ConditionalOutputProvider*>(
        (type == ConditionalBranchType::IfBranch) ? C_NAME_IF_OUT_NODE : C_NAME_ELSE_OUT_NODE);
}

ConditionalOutputProvider const*
ConditionalGroupNode::outputProvider(ConditionalBranchType type) const
{
    return const_cast<ConditionalGroupNode*>(this)->outputProvider(type);
}

Node::PortId
ConditionalGroupNode::addDataInPort(PortInfo info)
{
    return addInPort(std::move(info));
}

Node::PortId
ConditionalGroupNode::addDataOutPort(PortInfo info)
{
    return addOutPort(std::move(info));
}

bool
ConditionalGroupNode::isDataPort(PortId portId) const
{
    return portId.isValid() && portId > m_condition;
}

void
ConditionalGroupNode::updateDataPort(PortId portId, PortInfo newPort)
{
    if (!isDataPort(portId)) return;

    auto* port = this->port(portId);
    if (!port)
    {
        gtError() << relativeNodePath(*this) + QStringLiteral(":")
                  << tr("Failed to update data port %1! (Port not found)")
                         .arg(toString(portId));
        return;
    }

    port->assign(newPort);
    emit portChanged(portId);
}

void
ConditionalGroupNode::removeDataPort(PortId portId)
{
    if (!isDataPort(portId)) return;

    removePort(portId);
}

void
ConditionalGroupNode::initInputOutputProviders()
{
    for (auto type : { ConditionalBranchType::IfBranch, ConditionalBranchType::ElseBranch })
    {
        assert(inputProvider(type));
        assert(outputProvider(type));
    }
    return;
}

void
ConditionalGroupNode::eval()
{
    auto makeError = [this](){
        return gt::quoted(relativeNodePath(*this), "[", "] ") +
               tr("evaluation failed!");
    };

    // setup
    auto conditionData = nodeData<BoolData>(m_condition);
    if (!conditionData)
    {
        gtError() << makeError() << tr("unknown condition!");
        return evalFailed();
    }

    bool condition = conditionData->value();

    ConditionalInputProvider* inputNode = inputProvider(
        condition ? ConditionalBranchType::IfBranch : ConditionalBranchType::ElseBranch);
    ConditionalInputProvider* otherInputNode = inputProvider(
        condition ? ConditionalBranchType::ElseBranch : ConditionalBranchType::IfBranch);
    ConditionalOutputProvider* outputNode = outputProvider(
        condition ? ConditionalBranchType::IfBranch : ConditionalBranchType::ElseBranch);

    if (!inputNode || !outputNode || !otherInputNode)
    {
        gtError() << makeError() << tr("input/ouput providers not found!");
        return evalFailed();
    }

    auto* dataModel = qobject_cast<GraphDataModel*>(exec::nodeDataInterface(*this));
    if (!dataModel)
    {
        gtError() << makeError() << tr("data model not found!");
        return evalFailed();
    }

    // set input data
    for (NodePort const& port : ports(PortType::In))
    {
        if (port.id() == m_condition) continue;

        if (!inputNode->port(port.id()))
        {
            gtError() << makeError()
                      << tr("port '%1' in input provider not found!")
                             .arg(toString(port));
            return evalFailed();
        }

        if (!dataModel->setNodeData(inputNode->uuid(), port.id(), nodeData(port.id())))
        {
            gtError() << makeError()
                      << tr("failed to set input data for port '%1'!")
                             .arg(toString(port));
            return evalFailed();
        }
    }

    GtEventLoop loop{std::chrono::seconds{10}};

    // evaluate branch
    GraphExecutor executor{*this, *dataModel};

    loop.connectSuccess(&executor, &GraphExecutor::targetNodesEvaluated);
    loop.connectAbort(this, &Graph::graphAboutToBeDeleted);

    auto future = executor.evaluateNode(outputNode->id());
    dataModel->setNodeEvaluationFailed(otherInputNode->uuid());

    // TODO: cannot block main thread here!
    auto status = loop.exec();
    if (status != GtEventLoop::Success)
    {
        return evalFailed();
    }

    // set output data
    for (NodePort const& port : ports(PortType::Out))
    {
        if (port.id() == m_condition) continue;

        if (!outputNode->port(port.id()))
        {
            gtError() << makeError()
                      << tr("port '%1' in output provider not found!")
                             .arg(toString(port));
            return evalFailed();
        }

        if (!setNodeData(port.id(), dataModel->nodeData(outputNode->uuid(), port.id())))
        {
            gtError() << makeError()
                      << tr("failed to set output data for port '%1'!")
                             .arg(toString(port));
            return evalFailed();
        }
    }
}

void
ConditionalGroupNode::onObjectDataMerged()
{
    Graph::onObjectDataMerged();
}
