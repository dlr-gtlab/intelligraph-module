/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/graphstatemanager.h>

#include <intelli/graph.h>
#include <intelli/graphexecmodel.h>
#include <intelli/gui/graphview.h>
#include <intelli/gui/graphscene.h>
#include <intelli/gui/guidata.h>
#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/private/utils.h>

using namespace intelli;

GraphStateManager::GraphStateManager(Graph& graph, GraphView& view) :
    QObject((QObject*)&view),
    m_view(&view),
    m_guardian(make_unique_qptr<GtObject>())
{
    m_guardian->setParent((QObject*)&view);

    // grid visible state
    auto* v = &view;

    auto getGridVisible = std::bind(&GraphView::isGridVisible, v);

    utils::setupState<GraphView>(*m_guardian, graph, tr("Show Grid"), getGridVisible)
        .onStateChange(v, [v](QVariant const& show){ v->showGrid(show.toBool()); })
        .onValueChange(v, &GraphView::gridVisibilityChanged)
        .finalize();

    connect(m_view, &GraphView::sceneChanged,
            this, &GraphStateManager::onSceneChanged);

    if (GraphScene* scene = m_view->nodeScene())
    {
        onSceneChanged(scene);
    }
}

GraphStateManager::~GraphStateManager() = default;

GraphStateManager*
GraphStateManager::make(Graph& graph, GraphView &view)
{
    return new GraphStateManager(graph, view);
}

GtObject*
GraphStateManager::guardianObject()
{
    return m_guardian;
}

void
GraphStateManager::onSceneChanged(GraphScene* scene)
{
    assert(m_view);
    assert(m_guardian);
    if (!scene) return;

    setupUserStates(scene);

    setupLocalStates(scene);
}

void
GraphStateManager::setupUserStates(GraphScene* scene)
{
    auto& graph = scene->graph();
    auto& guardian = *m_guardian;

    // snap to grid state
    auto getSnapToGrid = std::bind(&GraphScene::snapToGrid, scene);

    utils::setupState<GraphScene>(guardian, graph, tr("Snap to Grid"), getSnapToGrid)
        .onStateChange(scene, [scene](QVariant const& enable) { scene->setSnapToGrid(enable.toBool()); })
        .onValueChange(scene, &GraphScene::snapToGridChanged)
        .finalize();

    // connection style state
    auto getConnectionShape = [scene](){ return (qulonglong)(scene->connectionShape()); };

    utils::setupState<GraphScene>(guardian, graph, tr("Connection Shape"), getConnectionShape)
        .onStateChange(scene, [scene](QVariant const& value) {
            // could not get Q_DECLARE_METATYPE to work
            ConnectionShape shape = ConnectionShape::DefaultShape;
            switch (value.value<qulonglong>())
            {
            case (qulonglong)ConnectionShape::Cubic:
                shape = ConnectionShape::Cubic;
                break;
            case (qulonglong)ConnectionShape::Rectangle:
                shape = ConnectionShape::Rectangle;
                break;
            case (qulonglong)ConnectionShape::Straight:
                shape = ConnectionShape::Straight;
                break;
            }
            scene->setConnectionShape(shape);
        })
        .onValueChange(scene, &GraphScene::connectionShapeChanged)
        .finalize();

    // auto evaluation state
    auto* model = GraphExecutionModel::accessExecModel(graph);
    if (!model) return;

    auto getAutoEvaluationEnabled =
        std::bind(qOverload<>(&GraphExecutionModel::isAutoEvaluatingGraph), model);

    utils::setupState<GraphExecutionModel>(guardian, graph, tr("Auto Evaluation"),
                                           getAutoEvaluationEnabled)
        .onStateChange(model, [model](QVariant const& enable) {
            bool autoEvaluate = enable.toBool();
            if (autoEvaluate == model->isAutoEvaluatingGraph()) return;

            autoEvaluate ? (void)model->autoEvaluateGraph() :
                (void)model->stopAutoEvaluatingGraph();
        })
        .onValueChange(model, &GraphExecutionModel::autoEvaluationChanged)
        .finalize();

}

void
GraphStateManager::setupLocalStates(GraphScene* scene)
{
    auto& graph = scene->graph();

    auto* localStates = GuiData::accessLocalStates(graph);
    if (!localStates) return;

    // collapsed state
    auto onStateChanged = [scene](QString const& nodeUuid, bool isCollapsed){
        Graph& graph = scene->graph();

        Node* node = graph.findNodeByUuid(nodeUuid);
        if (!node || node->parent() != &graph) return;

        NodeGraphicsObject* object = scene->nodeObject(node->id());
        if (!object) return;

        if (object->isCollpased() != isCollapsed)
        {
            object->collapse(isCollapsed);
        }
    };

    auto onNodeAppended = [scene, localStates](Node* node){
        assert(node);

        auto object = scene->nodeObject(node->id());
        if (!object) return;

        auto onValueChanged = [localStates](NodeGraphicsObject* object, bool isCollapsed){
            assert(object);
            localStates->setNodeCollapsed(object->node().uuid(), isCollapsed);
        };

        QObject::connect(object, &NodeGraphicsObject::nodeCollapsed,
                         localStates, onValueChanged);

        object->collapse(localStates->isNodeCollapsed(node->uuid()));
    };

    QObject::connect(localStates, &LocalStateContainer::nodeCollapsedChanged,
                     scene, onStateChanged);

    connect(&graph, &Graph::nodeAppended, this, onNodeAppended);

    // register existing nodes
    for (NodeId nodeId : graph.connectionModel().iterateNodeIds())
    {
        onNodeAppended(graph.findNode(nodeId));
    }
}
