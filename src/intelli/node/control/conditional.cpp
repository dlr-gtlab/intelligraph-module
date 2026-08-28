/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#include "intelli/node/control/conditional.h"

#include "intelli/data/bool.h"
#include "intelli/data/double.h"

#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"
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

/// helper struct to allow accessing protected methods
struct ConditionalGroupNode::Impl
{

template <typename ProviderA, typename ProviderB>
static void onPortInserted(ConditionalGroupNode* root,
                            ProviderA* ifProvider,
                           ProviderB* elseProvider,
                           PortType type,
                           PortIndex idx)
{
    assert(root);

    auto const makeError = [root, type, idx](){
        return relativeNodePath(*root) + QStringLiteral(": ") +
               QObject::tr("Failed to add %3 port (%1/%2)!")
                   .arg(toString(idx),
                        toString(type),
                        type == PortType::In ? "input":"output");
    };

    PortInfo* srcPort = root->port(root->portId(type, idx));
    if (!srcPort)
    {
        gtError() << makeError() << QObject::tr("(Source port not found)");
        return;
    }
    assert(root->isDataPort(srcPort->id()));

    for (auto* provider : { ifProvider, elseProvider })
    {
        if (!provider)
        {
            gtError() << makeError()
                      << QObject::tr("(%1-node not found)")
                             .arg(provider == ifProvider ? "if":"else");
            return;
        }

        PortId addedPortId = provider->addPort(*srcPort);
        if (!addedPortId.isValid())
        {
            gtError() << makeError()
                      << QObject::tr("(Adding %1-port failed)")
                             .arg(provider == ifProvider ? "if":"else");
            return;
        }
        assert(addedPortId == srcPort->id());
    }
}

template <typename ProviderA, typename ProviderB>
static void onPortChanged(ConditionalGroupNode* root,
                          ProviderA* ifProvider,
                          ProviderB* elseProvider,
                          PortType type,
                          PortId portId)
{
    assert(root);

    auto const makeError = [root, type, portId](){
        return relativeNodePath(*root) + QStringLiteral(": ") +
               tr("Failed to update %2 port (%1)!")
                   .arg(toString(portId),
                        type == PortType::In ? "input":"output");
    };

    assert(portId.isValid());

    PortInfo* srcPort = root->port(portId);
    if (!srcPort)
    {
        gtError() << makeError() << tr("(Source port not found)");
        return;
    }
    assert(root->isDataPort(portId));

    for (auto* provider : { ifProvider, elseProvider })
    {
        if (!provider)
        {
            gtError() << makeError()
                      << QObject::tr("(%1-node not found)")
                             .arg(provider == ifProvider ? "if":"else");
            return;
        }

        PortInfo* port = provider->port(srcPort->id());
        if (!port)
        {
            gtError() << makeError()
                      << QObject::tr("(Updating %1-port failed)")
                             .arg(provider == ifProvider ? "if":"else");
            return;
        }
        port->assign(*srcPort);
        emit provider->portChanged(port->id());
    }
}

template <typename ProviderA, typename ProviderB>
static void onPortDeleted(ConditionalGroupNode* root,
                          ProviderA* ifProvider,
                          ProviderB* elseProvider,
                          PortType type,
                          PortIndex idx)
{
    assert(root);

    auto const makeError = [root, type, idx](){
        return relativeNodePath(*root) + QStringLiteral(": ") +
               tr("Failed to delete %3 port (%1/%2)!")
                   .arg(toString(idx),
                        toString(type),
                        type == PortType::In ? "input":"output");
    };

    auto portId = root->portId(type, idx);
    if (!portId.isValid())
    {
        gtError() << makeError() << tr("(Source port not found)");
        return;
    }
    assert(root->isDataPort(portId));

    for (auto* provider : { ifProvider, elseProvider })
    {
        if (!provider)
        {
            gtError() << makeError()
                      << QObject::tr("(%1-node not found)")
                             .arg(provider == ifProvider ? "if":"else");
            return;
        }

        if (!provider->removePort(portId))
        {
            gtError() << makeError()
                      << QObject::tr("(Removing %1-port failed)")
                             .arg(provider == ifProvider ? "if":"else");
            return;
        }
    }
}

}; // struct Impl

ConditionalGroupNode::ConditionalGroupNode() :
    Graph(QStringLiteral("Conditional"))
{
    NodeId nextId{0};

    for (auto type : { IfBranch, ElseBranch })
    {
        Position offset{0, 100};
        if (type == ConditionalGroupNode::IfBranch) offset.ry() *= -1;

        auto input = std::make_unique<ConditionalInputProvider>();
        input->setCaption(type == ConditionalGroupNode::IfBranch ?
                              C_NAME_IF_IN_NODE : C_NAME_ELSE_IN_NODE);
        input->setPos(input->pos() + offset);
        input->setDefault(true);
        input->setId(nextId++);
        appendNode(std::move(input), NodeIdPolicy::Keep);

        auto output = std::make_unique<ConditionalOutputProvider>();
        output->setCaption(type == ConditionalGroupNode::IfBranch ?
                               C_NAME_IF_OUT_NODE : C_NAME_ELSE_OUT_NODE);
        output->setPos(output->pos() + offset);
        output->setDefault(true);
        output->setId(nextId++);
        appendNode(std::move(output), NodeIdPolicy::Keep);
    }

    // add static port first
    m_condition = addStaticInPort(
        PortInfo::customId(PortId{0}, typeId<BoolData>())
            .setCaption("condition")
            .setOptional(false));

    // sync dynamic ports
    connect(this, &Node::portInserted,
            this, &ConditionalGroupNode::onPortInserted,
            Qt::DirectConnection);
    connect(this, &Node::portChanged,
            this, &ConditionalGroupNode::onPortChanged,
            Qt::DirectConnection);
    connect(this, &Node::portAboutToBeDeleted,
            this, &ConditionalGroupNode::onPortDeleted,
            Qt::DirectConnection);

    addDataInPort(makePort(typeId<DoubleData>()));
    addDataOutPort(makePort(typeId<DoubleData>()));

    setNodeEvalMode(NodeEvalMode::Blocking);
}

ConditionalInputProvider*
ConditionalGroupNode::inputProvider(BranchType type)
{
    return findDirectChild<GraphInputProvider*>(
        (type == IfBranch) ? C_NAME_IF_IN_NODE : C_NAME_ELSE_IN_NODE);
}

ConditionalInputProvider const*
ConditionalGroupNode::inputProvider(BranchType type) const
{
    return const_cast<ConditionalGroupNode*>(this)->inputProvider(type);
}

ConditionalOutputProvider*
ConditionalGroupNode::outputProvider(BranchType type)
{
    return findDirectChild<GraphOutputProvider*>(
        (type == IfBranch) ? C_NAME_IF_OUT_NODE : C_NAME_ELSE_OUT_NODE);
}

ConditionalOutputProvider const*
ConditionalGroupNode::outputProvider(BranchType type) const
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
    for (auto type : { IfBranch, ElseBranch })
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

    ConditionalInputProvider* inputNode = inputProvider(condition ? IfBranch : ElseBranch);
    ConditionalInputProvider* otherInputNode = inputProvider(condition ? ElseBranch : IfBranch);
    ConditionalOutputProvider* outputNode = outputProvider(condition ? IfBranch : ElseBranch);

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

void
ConditionalGroupNode::onPortInserted(PortType type, PortIndex idx)
{
    auto cmd = modify();
    Q_UNUSED(cmd);
    if (type == PortType::In)
    {
        Impl::onPortInserted(this, inputProvider(IfBranch), inputProvider(ElseBranch), type, idx);
    }
    else
    {
        Impl::onPortInserted(this, outputProvider(IfBranch), outputProvider(ElseBranch), type, idx);
    }
}

void
ConditionalGroupNode::onPortChanged(PortId portId)
{
    auto cmd = modify();
    Q_UNUSED(cmd);
    PortType type = portType(portId);
    if (type == PortType::In)
    {
        Impl::onPortChanged(this, inputProvider(IfBranch), inputProvider(ElseBranch), type, portId);
    }
    else
    {
        Impl::onPortChanged(this, outputProvider(IfBranch), outputProvider(ElseBranch), type, portId);
    }
}

void
ConditionalGroupNode::onPortDeleted(PortType type, PortIndex idx)
{
    auto cmd = modify();
    Q_UNUSED(cmd);
    if (type == PortType::In)
    {
        Impl::onPortDeleted(this, inputProvider(IfBranch), inputProvider(ElseBranch), type, idx);
    }
    else
    {
        Impl::onPortDeleted(this, outputProvider(IfBranch), outputProvider(ElseBranch), type, idx);
    }
}
