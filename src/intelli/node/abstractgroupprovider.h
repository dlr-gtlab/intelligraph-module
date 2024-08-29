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

    PortId insertPort(PortInfo data, int idx = -1)
    {
        // visible output port (input provider) and input port (output provider)
        data = data.copy(m_nextPortId);

        PortId portId = (Type == PortType::In ? insertOutPort(data, idx) :
                                                insertInPort(data, idx));
        if (!portId.isValid())
        {
            return PortId{};
        }
        assert(isMainPort(portId));

        // hidden input port (input provider) and output port (output provider)
        data = data.copy(virtualPortId(m_nextPortId));
        data.visible = false;

        PortId virtualPortId = (Type == PortType::In ? insertInPort(data, idx) :
                                                       insertOutPort(data, idx));
        if (!virtualPortId.isValid())
        {
            bool success = removePort(portId);
            assert(success);
            return PortId{};
        }

        assert(!isMainPort(virtualPortId));

        m_nextPortId += NextPortIdOffset;
        return portId;
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

private slots:

    /**
     * @brief Insert port in parent graph node
     * @param idx Index of inserted port
     */
    void onPortInserted(PortType actualType, PortIndex idx)
    {
        if (Type == PortType::In && Type == actualType) return;

        PortId portId = this->portId(invert(Type), idx);
        if (Type == actualType) portId = virtualPortId(portId);

        auto* port = this->port(portId);
        if (!port) return;

        auto* graph = findParent<Graph*>();
        if (!graph) return;

        bool isHidden = !port->visible;

        actualType == PortType::Out ?
            graph->insertInPort( std::move(*port), isHidden ? -1 : idx) :
            graph->insertOutPort(std::move(*port), isHidden ? -1 : idx);
    }

    /**
     * @brief Update port in parent graph node
     * @param id Id of changed port
     */
    void onPortChanged(PortId portId)
    {
        if (Type == PortType::In && !isMainPort(portId)) return;

        auto* graph = findParent<Graph*>();
        if (!graph) return;

        auto* port = this->port(portId);
        if (!port) return;

        auto* graphPort  = graph->port(portId);
        if (!graphPort) return;

        *graphPort = *port;
        emit graph->portChanged(portId);
    }

    /**
     * @brief Delete port in parent graph node
     * @param idx Index of the deleted port
     */
    void onPortDeleted(PortType actualType, PortIndex idx)
    {
        if (Type == PortType::In && Type == actualType) return;

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
