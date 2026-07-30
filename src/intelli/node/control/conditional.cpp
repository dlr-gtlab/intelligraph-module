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

#include "intelli/private/utils.h"


using namespace intelli;

const char* C_NAME_IF_IN_NODE = "If Input";
const char* C_NAME_ELSE_IN_NODE = "Else Input";
const char* C_NAME_IF_OUT_NODE = "If Output";
const char* C_NAME_ELSE_OUT_NODE = "Else Output";

/// helper struct to allow accessing protected methods
struct ConditionalGroupNode::Impl
{

template <typename PorviderA, typename ProviderB>
static void onPortInserted(ConditionalGroupNode* root,
                           PorviderA* ifProvider,
                           ProviderB* elseProvider,
                           PortType actualType,
                           PortIndex idx)
{
    assert(root);

    PortType type = invert(actualType);

    auto const makeError = [root, type, idx](){
        return relativeNodePath(*root) + QStringLiteral(": ") +
               QObject::tr("Failed to add %3put port (%1/%2)!")
                   .arg(toString(idx),
                        toString(type),
                        type == PortType::Out ? "in":"out");
    };

    // if-node
    if (!ifProvider)
    {
        gtError() << makeError() << QObject::tr("(Source node not found)");
        return;
    }
    // else-node
    if (!elseProvider)
    {
        gtError() << makeError() << QObject::tr("(Else-node not found)");
        return;
    }

    PortInfo* srcPort = ifProvider->port(ifProvider->portId(actualType, idx));
    if (!srcPort)
    {
        gtError() << makeError() << QObject::tr("(Source port not found)");
        return;
    }
    assert(srcPort->id() > root->m_condition);

    PortId addedPortId = actualType == PortType::Out ?
                           root->addInPort(*srcPort) :
                           root->addOutPort(*srcPort);
    if (!addedPortId.isValid())
    {
        gtError() << makeError() << QObject::tr("(Adding port failed)");
        return;
    }
    assert(addedPortId == srcPort->id());

    addedPortId = elseProvider->addPort(srcPort->copy());
    if (!addedPortId.isValid())
    {
        gtError() << makeError() << tr("(Addig else-port failed)");
        return;
    }
    assert(addedPortId == srcPort->id());
}

template <PortType Type, typename PorviderA, typename ProviderB>
static void onPortChanged(ConditionalGroupNode* root,
                          PorviderA* ifProvider,
                          ProviderB* elseProvider,
                          PortId portId)
{
    assert(root);

    auto const makeError = [root, portId](){
        return relativeNodePath(*root) + QStringLiteral(": ") +
               tr("Failed to update %2put port (%1)!")
                   .arg(toString(portId),
                        Type == PortType::In ? "in":"out");
    };

    assert(portId.isValid());

    // if-node
    if (!ifProvider)
    {
        gtError() << makeError() << tr("(Source node not found)");
        return;
    }
    // else-node
    if (!elseProvider)
    {
        gtError() << makeError() << tr("(Else-node not found)");
        return;
    }

    PortInfo* srcPort = ifProvider->port(portId);
    if (!srcPort)
    {
        gtError() << makeError() << tr("(Source port not found)");
        return;
    }
    assert(portId > root->m_condition);

    // main-node
    auto* port = root->port(srcPort->id());
    if (!port)
    {
        gtError() << makeError() << tr("(Updating port failed)");
        return;
    }
    port->assign(*srcPort);
    emit root->portChanged(port->id());

    port = elseProvider->port(srcPort->id());
    if (!port)
    {
        gtError() << makeError() << tr("(Updating else-port failed)");
        return;
    }
    port->assign(*srcPort);
    emit elseProvider->portChanged(port->id());
}

template <typename PorviderA, typename ProviderB>
static void onPortDeleted(ConditionalGroupNode* root,
                          PorviderA* ifProvider,
                          ProviderB* elseProvider,
                          PortType actualType,
                          PortIndex idx)
{
    assert(root);

    PortType type = invert(actualType);

    auto const makeError = [root, type, idx](){
        return relativeNodePath(*root) + QStringLiteral(": ") +
               tr("Failed to delete %3put port (%1/%2)!")
                   .arg(toString(idx),
                        toString(type),
                        type == PortType::Out ? "in":"out");
    };

    // if-node
    if (!ifProvider)
    {
        gtError() << makeError() << tr("(Source node not found)");
        return;
    }
    // else-node
    if (!elseProvider)
    {
        gtError() << makeError() << tr("(Else-node not found)");
        return;
    }

    auto portId = ifProvider->portId(actualType, idx);
    if (!portId.isValid())
    {
        gtError() << makeError() << tr("(Source port not found)");
        return;
    }
    assert(portId > root->m_condition);

    // main-node
    if (!root->port(portId))
    {
        gtError() << makeError() << tr("(Removing port failed)");
        return;
    }
    root->removePort(portId);

    if (!elseProvider->port(portId))
    {
        gtError() << makeError() << tr("(Removing else-port failed)");
        return;
    }
    elseProvider->removePort(portId);
}

}; // struct Impl

ConditionalGroupNode::ConditionalGroupNode() :
    Graph(QStringLiteral("Conditional"))
{
    for (auto type : { IfBranch, ElseBranch })
    {
        auto input = std::make_unique<ConditionalInputProvider>(type);
        input->setDefault(true);
        appendNode(std::move(input));

        auto output = std::make_unique<ConditionalOutputProvider>(type);
        output->setDefault(true);
        appendNode(std::move(output));
    }

    /// only the if branch nodes are used to synchronize the state across all
    /// other nodes
    if (auto* input = inputProvider(IfBranch))
    {
        connect(input, &Node::portInserted,
                this, &ConditionalGroupNode::onInPortInserted,
                Qt::DirectConnection);
        connect(input, &Node::portChanged,
                this, &ConditionalGroupNode::onInPortChanged,
                Qt::DirectConnection);
        connect(input, &Node::portAboutToBeDeleted,
                this, &ConditionalGroupNode::onInPortDeleted,
                Qt::DirectConnection);
    }
    if (auto* output = outputProvider(IfBranch))
    {
        connect(output, &Node::portInserted,
                this, &ConditionalGroupNode::onOutPortInserted,
                Qt::DirectConnection);
        connect(output, &Node::portChanged,
                this, &ConditionalGroupNode::onOutPortChanged,
                Qt::DirectConnection);
        connect(output, &Node::portAboutToBeDeleted,
                this, &ConditionalGroupNode::onOutPortDeleted,
                Qt::DirectConnection);
    }

    m_condition = addInPort(PortInfo::customId(PortId{0}, typeId<BoolData>())
                                .setCaption("condition"));

    addDataInPort(makePort(typeId<DoubleData>()));
    addDataOutPort(makePort(typeId<DoubleData>()));
}

ConditionalInputProvider*
ConditionalGroupNode::inputProvider(BranchType type)
{
    return findDirectChild<ConditionalInputProvider*>(
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
    return findDirectChild<ConditionalOutputProvider*>(
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
    auto* provider = inputProvider(IfBranch);
    assert(provider);

    return provider->addPort(std::move(info));
}

Node::PortId
ConditionalGroupNode::addDataOutPort(PortInfo info)
{
    auto* provider = outputProvider(IfBranch);
    assert(provider);

    return provider->addPort(std::move(info));
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

    PortType type = portType(portId);

    auto* provider = type == PortType::In ?
                         static_cast<DynamicNode*>(inputProvider(IfBranch)) :
                         static_cast<DynamicNode*>(outputProvider(IfBranch));
    if (!provider)
    {
        gtError() << relativeNodePath(*this) + QStringLiteral(":")
                  << tr("Failed to update data port %1! (Node not found)")
                         .arg(toString(portId));
        return;
    }

    auto* port = provider->port(portId);
    if (!port)
    {
        gtError() << relativeNodePath(*this) + QStringLiteral(":")
                  << tr("Failed to update data port %1! (Port not found)")
                         .arg(toString(portId));
        return;
    }

    port->assign(newPort);
    emit provider->portChanged(portId);
}

void
ConditionalGroupNode::deleteDataPort(PortId portId)
{
    if (!isDataPort(portId)) return;

    PortType type = portType(portId);

    auto* provider = type == PortType::In ?
                         static_cast<DynamicNode*>(inputProvider(IfBranch)) :
                         static_cast<DynamicNode*>(outputProvider(IfBranch));
    if (!provider)
    {
        gtError() << relativeNodePath(*this) + QStringLiteral(":")
                  << tr("Failed to delete data port %1! (Node not found)")
                         .arg(toString(portId));
        return;
    }

    provider->removePort(portId);
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
ConditionalGroupNode::onInPortInserted(PortType actualType, PortIndex idx)
{
    assert(actualType == PortType::Out);
    Impl::onPortInserted(this, inputProvider(IfBranch), inputProvider(ElseBranch), actualType, idx);
}

void
ConditionalGroupNode::onInPortChanged(PortId portId)
{
    Impl::onPortChanged<PortType::In>(this, inputProvider(IfBranch), inputProvider(ElseBranch), portId);
}

void
ConditionalGroupNode::onInPortDeleted(PortType actualType, PortIndex idx)
{
    assert(actualType == PortType::Out);
    Impl::onPortDeleted(this, inputProvider(IfBranch), inputProvider(ElseBranch), actualType, idx);
}

void
ConditionalGroupNode::onOutPortInserted(PortType actualType, PortIndex idx)
{
    assert(actualType == PortType::In);
    Impl::onPortInserted(this, outputProvider(IfBranch), outputProvider(ElseBranch), actualType, idx);
}

void
ConditionalGroupNode::onOutPortChanged(PortId portId)
{
    Impl::onPortChanged<PortType::Out>(this, outputProvider(IfBranch), outputProvider(ElseBranch), portId);
}

void
ConditionalGroupNode::onOutPortDeleted(PortType actualType, PortIndex idx)
{
    assert(actualType == PortType::In);
    Impl::onPortDeleted(this, outputProvider(IfBranch), outputProvider(ElseBranch), actualType, idx);
}


ConditionalInputProvider::ConditionalInputProvider(ConditionalGroupNode::BranchType type) :
    GroupInputProvider(type == ConditionalGroupNode::IfBranch ? C_NAME_IF_IN_NODE : C_NAME_ELSE_IN_NODE)
{
    QPointF position = pos();
    position.ry() += (type == ConditionalGroupNode::IfBranch ? -1 : 1) * 100;
    setPos(position);

    setNodeFlag(Unique, false);
    setUseVirtualPorts(false);
    setPortContainerVisible(PortType::Out, false);
}


ConditionalOutputProvider::ConditionalOutputProvider(ConditionalGroupNode::BranchType type) :
    GroupOutputProvider(type == ConditionalGroupNode::IfBranch ? C_NAME_IF_OUT_NODE : C_NAME_ELSE_OUT_NODE)
{
    QPointF position = pos();
    position.ry() += (type == ConditionalGroupNode::IfBranch ? -1 : 1) * 100;
    setPos(position);

    setNodeFlag(Unique, false);
    setUseVirtualPorts(false);
    setPortContainerVisible(PortType::In, false);
}
