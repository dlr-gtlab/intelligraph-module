/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/graphscenemanager.h>

#include <intelli/graph.h>
#include <intelli/gui/graphview.h>
#include <intelli/gui/graphscene.h>

using namespace intelli;

namespace
{

inline auto findGraphOp(Graph& graph)
{
    return [&graph](auto const& scene){
        return scene && &scene->graph() == &graph;
    };
}

} // namespace

GraphSceneManager::GraphSceneManager(GraphView& view) :
    QObject((QObject*)&view),
    m_view(&view)
{

}

GraphSceneManager*
GraphSceneManager::make(GraphView& view)
{
    return new GraphSceneManager(view);
}

GraphScene*
GraphSceneManager::currentScene()
{
    assert(m_view);
    return m_view->nodeScene();
}

GraphScene const*
GraphSceneManager::currentScene() const
{
    return const_cast<GraphSceneManager*>(this)->currentScene();
}

GraphScene*
GraphSceneManager::createScene(Graph& graph)
{
    assert(m_view);

    // if graph has scene -> abort
    auto iter = std::find_if(m_scenes.cbegin(), m_scenes.cend(), findGraphOp(graph));
    if (iter != m_scenes.end())
    {
        gtError() << QStringLiteral("[%1]").arg(GT_CLASSNAME(GraphSceneManager))
                  << tr("Failed to create scene for graph '%1'!")
                         .arg(relativeNodePath(graph));
        return {};
    }

    // create scene
    auto scenePtr = make_unique_qptr<GraphScene, DirectDeleter>(graph);
    GraphScene* scene = scenePtr.get();

    m_scenes.push_back(std::move(scenePtr));

    connect(scene, &GraphScene::graphNodeDoubleClicked,
            this, &GraphSceneManager::openGraph);

    // if view has no scene -> set scene
    if (!m_view->nodeScene()) m_view->setScene(*scene);

    return scene;
}

void
GraphSceneManager::openGraph(Graph* graph)
{
    if (!graph)
    {
        gtError() << QStringLiteral("[%1]").arg(GT_CLASSNAME(GraphSceneManager))
                  << tr("Failed to open graph! (null graph)");
        return;
    }

    GraphScene* scene = nullptr;

    auto iter = std::find_if(m_scenes.begin(), m_scenes.end(), findGraphOp(*graph));
    if (iter == m_scenes.cend())
    {
        scene = createScene(*graph);
    }
    else
    {
        scene = *iter;
    }

    if (!scene)
    {
        gtError() << QStringLiteral("[%1]").arg(GT_CLASSNAME(GraphSceneManager))
                  << tr("Failed to open graph! (null scene)");
        return;
    }

    if (scene == currentScene()) return;

    // switch scene
    m_view->setScene(*scene);
}

void
GraphSceneManager::openGraphByUuid(QString const& graphUuid)
{
    assert(currentScene());

    auto* root = currentScene()->graph().rootGraph();
    assert(root);

    auto* graph = qobject_cast<Graph*>(root->findNodeByUuid(graphUuid));
    if (!graph)
    {
        gtError() << QStringLiteral("[%1]").arg(GT_CLASSNAME(GraphSceneManager))
                  << tr("Failed to open graph by uuid! "
                        "(uuid '%1' not found in graph '%2'!")
                         .arg(graphUuid, root->caption());
        return;
    }

    openGraph(graph);
}
