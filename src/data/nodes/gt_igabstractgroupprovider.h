/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_IGABSTRACTGROUPPROVIDER_H
#define GT_IGABSTRACTGROUPPROVIDER_H

#include "gt_coreapplication.h"
#include "gt_intelligraph.h"
#include "gt_intelligraphdynamicnode.h"

template <gt::ig::PortType Type>
class GtIgAbstractGroupProvider : public GtIntelliGraphDynamicNode
{
public:

    static constexpr inline PortType INVERSE_TYPE() noexcept
    {
        return Type == PortType::In ? PortType::Out : PortType::In;
    }

    static constexpr inline PortType TYPE() noexcept
    {
        return Type;
    }

    GtIgAbstractGroupProvider(QString const& modelName) :
        GtIntelliGraphDynamicNode(modelName, Type == PortType::In ? DynamicOutputOnly : DynamicInputOnly)
    {
        setId(NodeId{static_cast<int>(Type)});
        setFlag(UserDeletable, false);
        setNodeFlag(NodeFlag::Unique, true);

        if (!gtApp || !gtApp->devMode()) setFlag(UserHidden, true);

        connect(this, &GtIntelliGraphNode::portInserted,
                this, &GtIgAbstractGroupProvider::onPortInserted,
                Qt::UniqueConnection);
        connect(this, &GtIntelliGraphNode::portChanged,
                this, &GtIgAbstractGroupProvider::onPortChanged,
                Qt::UniqueConnection);
        connect(this, &GtIntelliGraphNode::portAboutToBeDeleted,
                this, &GtIgAbstractGroupProvider::onPortDeleted,
                Qt::UniqueConnection);
    }

    bool insertPort(PortData data, int idx = -1)
    {
        return GtIntelliGraphDynamicNode::insertPort(INVERSE_TYPE(), data, idx);
    }

private:

    // "hide" unused methods
    using GtIntelliGraphNode::addInPort;
    using GtIntelliGraphNode::addOutPort;
    using GtIntelliGraphNode::insertInPort;
    using GtIntelliGraphNode::insertOutPort;
    using GtIntelliGraphNode::insertPort;

private slots:

    void onPortInserted(PortType, PortIndex idx)
    {
        auto* graph = findParent<GtIntelliGraph*>();
        if (!graph) return;

        PortId id = portId(INVERSE_TYPE(), idx);
        if (auto* port = this->port(id))
        {
            Type == PortType::In ?
                graph->insertInPort(*port, idx) :
                graph->insertOutPort(*port, idx);
        }
    }

    void onPortChanged(PortId id)
    {
        auto* graph = findParent<GtIntelliGraph*>();
        if (!graph) return;

        auto* port = this->port(id);
        if (!port) return;

        auto  idx    = portIndex(INVERSE_TYPE(), id);
        auto* graphPort   = graph->port(graph->portId(Type, idx));
        if (!graphPort) return;

        graphPort->typeId = port->typeId;
        graphPort->caption = port->caption;
        emit graph->portChanged(graphPort->id());
    }

    void onPortDeleted(PortType, PortIndex idx)
    {
        auto* graph = findParent<GtIntelliGraph*>();
        if (!graph) return;

        graph->removePort(graph->portId(Type, idx));
    }
};

// disbale template class for none type
template <> class GtIgAbstractGroupProvider<gt::ig::NoType>;

#endif // GT_IGABSTRACTGROUPPROVIDER_H
