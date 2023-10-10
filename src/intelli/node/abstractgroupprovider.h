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

    AbstractGroupProvider(QString const& modelName) :
        DynamicNode(modelName, Type == PortType::In ? DynamicOutputOnly : DynamicInputOnly)
    {
        setFlag(UserDeletable, false);

        setNodeFlag(Unique, true);

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

    bool insertPort(PortData data, int idx = -1)
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

    void onPortInserted(PortType, PortIndex idx)
    {
        auto* graph = findParent<Graph*>();
        if (!graph) return;

        PortId id = portId(invert(Type), idx);
        if (auto* port = this->port(id))
        {
            Type == PortType::In ?
                graph->insertInPort(*port, idx) :
                graph->insertOutPort(*port, idx);
        }
    }

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

        graphPort->typeId  = port->typeId;
        graphPort->caption = port->caption;
        emit graph->portChanged(graphPortId);
    }

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
