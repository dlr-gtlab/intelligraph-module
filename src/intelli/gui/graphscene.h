/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_SCENE_H
#define GT_INTELLI_SCENE_H

#include "intelli/graph.h"

#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/Definitions>

namespace intelli
{

class GraphScene : public QtNodes::DataFlowGraphicsScene
{
    Q_OBJECT

    using base_class = QtNodes::DataFlowGraphicsScene;

public:

    GraphScene(Graph& graph);

public slots:

    void deleteSelectedObjects();

    void duplicateSelectedObjects();

    bool copySelectedObjects();

    void pasteObjects();

protected:

    void keyPressEvent(QKeyEvent* event) override;

private:

    QPointer<Graph> m_data = nullptr;

    QPointer<QtNodes::AbstractGraphModel> m_model = nullptr;

    void deleteNodes(std::vector<QtNodes::NodeId> const& nodeIds);

    void makeGroupNode(std::vector<QtNodes::NodeId> const& selectedNodeIds);

private slots:

    void onNodePositionChanged(QtNodes::NodeId nodeId);

    void onNodeSelected(QtNodes::NodeId nodeId);

    void onNodeDoubleClicked(QtNodes::NodeId nodeId);

    void onWidgetResized(QtNodes::NodeId nodeId, QSize size);

    void onNodeContextMenu(QtNodes::NodeId nodeId, QPointF pos);

    void onPortContextMenu(QtNodes::NodeId nodeId,
                           QtNodes::PortType type,
                           QtNodes::PortIndex idx,
                           QPointF pos);
};

} // namespace intelli

#endif // GT_INTELLI_SCENE_H
