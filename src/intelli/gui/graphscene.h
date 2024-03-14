/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_SCENE_H
#define GT_INTELLI_SCENE_H

#include <intelli/memory.h>
#include <intelli/graph.h>
#include <intelli/gui/graphics/nodeobject.h>

#include <QtNodes/BasicGraphicsScene>
#include <QtNodes/Definitions>

#include <map>

namespace intelli
{

class GraphAdapterModel;

class GraphScene : public QGraphicsScene//QtNodes::BasicGraphicsScene
{
    Q_OBJECT

public:

    GraphScene(Graph& graph);
    ~GraphScene();

    void reset();

    Graph& graph();
    Graph const& graph() const;

    void autoEvaluate(bool enable = true);

    bool isAutoEvaluating();

    QVector<Node*> selectedNodes();
    QVector<Node const*> selectedNodes() const;

    QMenu* createSceneMenu(QPointF scenePos) /*override*/;

public slots:

    void deleteSelectedObjects();

    void duplicateSelectedObjects();

    bool copySelectedObjects();

    void pasteObjects();

protected:

    void keyPressEvent(QKeyEvent* event) override;

private:

    struct Impl;
    
    QPointer<Graph> m_graph = nullptr;

    std::map<NodeId, volatile_ptr<NodeGraphicsObject>> m_nodes;

    void beginReset();

    void endReset();

    void makeGroupNode(std::vector<QtNodes::NodeId> const& selectedNodeIds);

//    GraphAdapterModel& adapterModel();

private slots:

    void onNodeAppended(Node* node);

    void onNodeDeleted(NodeId nodeId);

    void onNodeEvalStateChanged(NodeId nodeId);

    void onNodeContextMenu(Node* node, QPointF pos);

    void onPortContextMenu(Node* node, PortId portId, QPointF pos);
};

} // namespace intelli

#endif // GT_INTELLI_SCENE_H
