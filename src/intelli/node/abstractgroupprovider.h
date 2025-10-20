/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_ABSTRACTGROUPPROVIDER_H
#define GT_INTELLI_ABSTRACTGROUPPROVIDER_H

#include <intelli/graph.h>
#include <intelli/dynamicnode.h>

#include <gt_coreapplication.h>

namespace intelli
{

template <PortType Type>
class AbstractGroupProvider : public DynamicNode
{
    /// Input and output provider will have mutually exclusive port ids
    /// The initial offset is calculated like this
    PortId m_nextPortId{(size_t)Type + 1};

public:

    constexpr static PortType providerType = Type;

    AbstractGroupProvider(QString const& modelName,
                          QStringList iwl = {},
                          QStringList owl = {}) :
        DynamicNode(modelName, std::move(iwl), std::move(owl),
                    providerType == PortType::In ? DynamicOutput : DynamicInput)
    {
        setFlag(UserDeletable, false);
        setNodeFlag(Unique, true);

        setNodeEvalMode(NodeEvalMode::ForwardInputsToOutputs);

        if (!gtApp || !gtApp->devMode()) setFlag(UserHidden, true);

        connect(this, &Node::portInserted,
                this, &AbstractGroupProvider::onPortInserted,
                Qt::DirectConnection);
        connect(this, &Node::portChanged,
                this, &AbstractGroupProvider::onPortChanged,
                Qt::DirectConnection);
        connect(this, &Node::portAboutToBeDeleted,
                this, &AbstractGroupProvider::onPortDeleted,
                Qt::DirectConnection);
    }

    PortId addPort(PortInfo port, int idx = -1)
    {
        return insertPort(std::move(port));
    }

    PortId insertPort(PortInfo data, int idx = -1)
    {
        return insertPort(DynamicPort, invert(providerType), std::move(data), idx);
    }

    static constexpr PortId virtualPortId(PortId portId)
    {
        assert(isMainPort(portId));
        return portId + PortId(2);
    }

    static constexpr PortId mainPortId(PortId portId)
    {
        assert(!isMainPort(portId));
        return portId - PortId(2);
    }

    /**
     * @brief Returns whether the given port is the one intended for the
     * subgraph or is a "virtual" port to connect with parent graph.
     * @param portId
     * @return
     */
    static constexpr bool isMainPort(PortId portId)
    {
        return ((portId - ((size_t)Type + 1)) % 4) == 0;
    }

protected:

    PortId insertPort(PortOption option, PortType type, PortInfo port, int idx) final
    {
        // no static ports allowed
        if (option == StaticPort) return PortId{};
        // no custom port ids allowed
        if (port.id().isValid()) return PortId{};
        // invalid port type
        if (type == providerType) return PortId{};

        port = PortInfo::customId(m_nextPortId, std::move(port));
        PortId portId =
            DynamicNode::insertPort(option, type, port, idx);

        return portId;
    }

    PortId verifyPortId(PortId portId)
    {
        if (portId.isValid() || !isMainPort(portId))
        {
            portId = m_nextPortId;
        }
        return portId;
    }

private slots:

    /**
     * @brief Insert port in parent graph node
     * @param idx Index of inserted port
     */
    void onPortInserted(PortType actualType, PortIndex idx)
    {
        // port is virtual -> ignore
        if (providerType == actualType) return;

        PortId portId  = this->portId(actualType, idx);
        PortInfo* port = this->port(portId);
        assert(port);
        assert(isMainPort(portId));

        auto onFailure = gt::finally([this, portId](){
            bool success = removePort(portId);
            assert(success);
        });

        // update next port id
        m_nextPortId = std::max(m_nextPortId, portId);

        // append main port to graph
        auto* graph = findParent<Graph*>();
        assert(graph);

        PortId addedPortId = actualType == PortType::Out ?
                           graph->insertInPort( *port, idx) :
                           graph->insertOutPort(*port, idx);
        assert(addedPortId.isValid());

        port = this->port(addedPortId);
        // generate virtual port for connecting parent graph and this provider
        bool success = generateVirtualPort(graph, port, idx);
        if (success) onFailure.clear();
    }

    bool generateVirtualPort(Graph* graph, PortInfo* port, PortIndex idx)
    {
        PortInfo virtualPort = PortInfo::customId(virtualPortId(port->id()), *port);
        virtualPort.visible = false;
        PortId virtualPortId =
            DynamicNode::insertPort(StaticPort, providerType, virtualPort, idx);

        if (!virtualPortId.isValid()) return false;

        assert(!isMainPort(virtualPortId));

        // update next port id
        m_nextPortId += PortId(4);

        if (Type == PortType::Out)
        {
            bool success = graph->insertInPort(std::move(virtualPort), -1);
            assert(success);
        }

        return true;
    }

    /**
     * @brief Update port in parent graph node
     * @param id Id of changed port
     */
    void onPortChanged(PortId portId)
    {
        bool isVirtual = !isMainPort(portId);
        if (isVirtual) return;

        PortId virtualPortId = this->virtualPortId(portId);

        auto* port = this->port(portId);
        if (!port) return;

        auto* graph = findParent<Graph*>();
        assert(graph);

        auto* virtualPort = this->port(virtualPortId);
        auto* graphPort   = graph->port(portId);
        if (!virtualPort || !graphPort) return;

        // update all ports
        virtualPort->assign(*port);
        virtualPort->visible = false;
        graphPort->assign(*port);

        emit this->portChanged(virtualPortId);
        emit graph->portChanged(portId);

        if (Type == PortType::Out)
        {
            auto* graphVirtualPort  = graph->port(virtualPortId);
            if (!graphVirtualPort) return;

            graphVirtualPort->assign(*port);
            graphVirtualPort->visible = false;

            emit graph->portChanged(virtualPortId);
        }
    }

    /**
     * @brief Delete port in parent graph node
     * @param idx Index of the deleted port
     */
    void onPortDeleted(PortType actualType, PortIndex idx)
    {
        auto* graph = findParent<Graph*>();
        if (!graph) return;

        PortId portId = this->portId(actualType, idx);
        assert(portId.isValid());

        if (isMainPort(portId))
        {
            PortId virtualPortId = this->virtualPortId(portId);
            removePort(virtualPortId);
        }

        graph->removePort(portId);
    }
};

// disbale template class for other types
template <>
class AbstractGroupProvider<PortType::NoType>;

} // namespace intelli

#endif // GT_INTELLI_ABSTRACTGROUPPROVIDER_H
