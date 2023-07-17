/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTINTELLIGRAPHSCENE_H
#define GTINTELLIGRAPHSCENE_H

#include "gt_intelligraph.h"

#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/Definitions>

class GtIntelliGraphScene : public QtNodes::DataFlowGraphicsScene
{
    Q_OBJECT

public:

    GtIntelliGraphScene(GtIntelliGraph& graph, QObject* parent = nullptr);

public slots:

    void deleteSelectedObjects();

    void duplicateSelectedObjects();

    void copySelectedObjects();

    void pasteObjects();

private:

    QPointer<GtIntelliGraph> m_data = nullptr;

    QPointer<QtNodes::DataFlowGraphModel> m_model = nullptr;

    void deleteNodes(std::vector<QtNodes::NodeId> const& nodeIds);

    void makeGroupNode(std::vector<QtNodes::NodeId> const& selectedNodeIds);

private slots:

    void onNodePositionChanged(QtNodes::NodeId nodeId);

    void onNodeSelected(QtNodes::NodeId nodeId);

    void onNodeDoubleClicked(QtNodes::NodeId nodeId);

    void onNodeContextMenu(QtNodes::NodeId nodeId, QPointF pos);

    void onPortContextMenu(QtNodes::NodeId nodeId,
                           QtNodes::PortType type,
                           QtNodes::PortIndex idx,
                           QPointF pos);
};

#endif // GTINTELLIGRAPHSCENE_H
