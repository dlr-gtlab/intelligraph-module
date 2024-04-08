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
public:

    AbstractGroupProvider(QString const& modelName,
                          QStringList iwl = {},
                          QStringList owl = {}) :
        DynamicNode(modelName, std::move(iwl), std::move(owl),
                    Type == PortType::In ? DynamicOutputOnly : DynamicInputOnly)
    {
        setFlag(UserDeletable, false);

        setNodeFlag(Unique, true);

        setNodeEvalMode(NodeEvalMode::MainThread);

        if (!gtApp || !gtApp->devMode()) setFlag(UserHidden, true);

        connect(this, &Node::portInserted,
                this, &AbstractGroupProvider::onPortInserted,
                Qt::UniqueConnection);
        connect(this, &Node::portChanged,
                this, &AbstractGroupProvider::onPortChanged,
                Qt::UniqueConnection);
        connect(this, &Node::portAboutToBeDeleted,
                this, &AbstractGroupProvider::onPortDeleted,
                Qt::UniqueConnection);
    }

    bool insertPort(PortInfo data, int idx = -1)
    {
        PortId id;
        switch (invert(Type))
        {
        case PortType::In:
            id = insertInPort(data, idx);
            break;
        case PortType::Out:
            id = insertOutPort(data, idx);
            break;
        }

        return id != invalid<PortId>();
    }

private slots:

    /**
     * @brief Insert port in parent graph node
     * @param idx Index of inserted port
     */
    void onPortInserted(PortType, PortIndex idx)
    {
        auto* graph = findParent<Graph*>();
        if (!graph) return;

        PortId id = portId(invert(Type), idx);
        if (auto* port = this->port(id))
        {
            // generate new port id:
            //  odd port id  -> input port
            //  even port id -> output port
            auto graphPortId = id << 1;
            if (Type == PortType::In) graphPortId |= 1;

            auto p = PortInfo::customId(PortId(graphPortId), *port);

            Type == PortType::In ?
                graph->insertInPort( std::move(p), idx) :
                graph->insertOutPort(std::move(p), idx);
        }
    }

    /**
     * @brief Update port in parent graph node
     * @param id Id of changed port
     */
    void onPortChanged(PortId id)
    {
        auto* graph = findParent<Graph*>();
        if (!graph) return;

        auto* port = this->port(id);
        if (!port) return;

        auto idx = portIndex(invert(Type), id);
        auto graphPortId = graph->portId(Type, idx);

        auto* graphPort  = graph->port(graphPortId);
        if (!graphPort) return;

        // update port but keep port id
        *graphPort = PortInfo::customId(graphPortId, *port);

        emit graph->portChanged(graphPortId);
    }

    /**
     * @brief Delete port in parent graph node
     * @param idx Index of the deleted port
     */
    void onPortDeleted(PortType, PortIndex idx)
    {
        auto* graph = findParent<Graph*>();
        if (!graph) return;

        graph->removePort(graph->portId(Type, idx));
    }
};

// disbale template class for none type
template <>
class AbstractGroupProvider<PortType::NoType>;

} // namespace intelli

#endif // GT_INTELLI_ABSTRACTGROUPPROVIDER_H
