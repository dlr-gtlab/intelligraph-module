/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
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
    static constexpr PortId InitialPortId{(size_t)Type + 1};
    static constexpr PortId VirtualPortIdOffset{2};
    static constexpr PortId NextPortIdOffset{2 * VirtualPortIdOffset};

    /// Input and output provider will have mutually exclusive port ids
    /// The initial offset is calculated like this
    PortId m_nextPortId{InitialPortId};

public:

    constexpr static PortType providerType = Type;

    AbstractGroupProvider(QString const& modelName,
                          QStringList iwl = {},
                          QStringList owl = {}) :
        DynamicNode(modelName, std::move(iwl), std::move(owl),
                    Type == PortType::In ? DynamicOutputOnly : DynamicInputOnly)
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
        return insertPort(DynamicPort, invert(Type), std::move(data), idx);
    }

    static constexpr PortId virtualPortId(PortId portId)
    {
        assert(isMainPort(portId));
        return portId + VirtualPortIdOffset;
    }

    static constexpr PortId mainPortId(PortId portId)
    {
        assert(!isMainPort(portId));
        return portId - VirtualPortIdOffset;
    }

    /**
     * @brief Returns whether the given port is the one intended for the
     * subgraph or is a "virtual" port to connect with parent graph.
     * @param portId
     * @return
     */
    static constexpr bool isMainPort(PortId portId)
    {
        return ((portId - InitialPortId) % NextPortIdOffset) == 0;
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

        if (!portId.isValid()) return PortId{};

        //        assert(portId == m_nextPortId);

//        port = PortInfo::customId(virtualPortId(m_nextPortId), std::move(port));
//        PortId virtualPortId =
//            DynamicNode::insertPort(StaticPort, invert(type), std::move(port), idx);

//        if (!virtualPortId.isValid())
//        {
//            bool success = removePort(portId);
//            assert(success);
//            return PortId{};
//        }
//        assert(!isMainPort(virtualPortId));

//        m_nextPortId += NextPortIdOffset;
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
        if (Type == actualType) return;

        PortId portId = this->portId(actualType, idx);

        auto* port = this->port(portId);
        if (!port) return;
        assert(isMainPort(portId));

        // update next port id
        m_nextPortId = std::max(m_nextPortId, portId);

        // append main port to graph
        auto* graph = findParent<Graph*>();
        if (!graph) return;

        bool success = true;
        success = actualType == PortType::Out ?
                      graph->insertInPort( *port, idx) :
                      graph->insertOutPort(*port, idx);
        assert(success);

        // generate virtual port for connection parent graph and this graph
        PortInfo virtualPort = PortInfo::customId(virtualPortId(m_nextPortId), *port);
        virtualPort.visible = false;
        PortId virtualPortId =
            DynamicNode::insertPort(StaticPort, invert(actualType), virtualPort, idx);

        if (!virtualPortId.isValid())
        {
            success = removePort(portId);
            assert(success);
            return;
        }
        assert(!isMainPort(virtualPortId));
        m_nextPortId += NextPortIdOffset;

        // append virtual port to graph
        success = actualType == PortType::Out ?
                      graph->insertOutPort(std::move(virtualPort), -1) :
                      graph->insertInPort( std::move(virtualPort), -1);
        assert(success);
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
        if (!graph) return;

        auto* virtualPort = this->port(virtualPortId);
        auto* graphPort   = graph->port(portId);
        auto* graphVirtualPort  = graph->port(virtualPortId);

        if (!virtualPort || !graphPort || !graphVirtualPort) return;

        // update all ports
        virtualPort->assign(*port);
        virtualPort->visible = false;
        graphPort->assign(*port);
        graphVirtualPort->assign(*port);
        graphVirtualPort->visible = false;

        emit this->portChanged(virtualPortId);
        emit graph->portChanged(portId);
        emit graph->portChanged(virtualPortId);
    }

    /**
     * @brief Delete port in parent graph node
     * @param idx Index of the deleted port
     */
    void onPortDeleted(PortType actualType, PortIndex idx)
    {
        auto* graph = findParent<Graph*>();
        if (!graph) return;

        graph->removePort(portId(actualType, idx));
    }
};

// disbale template class for other types
template <>
class AbstractGroupProvider<PortType::NoType>;

} // namespace intelli

#endif // GT_INTELLI_ABSTRACTGROUPPROVIDER_H
