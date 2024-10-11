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
GraphSceneManager::createScene(Graph& graph)
{
    assert(m_view);

    // if graph has scene -> abort
    auto iter = std::find_if(m_scenes.cbegin(), m_scenes.cend(),
                             [&graph](auto const& scene){
        return scene && &scene->graph() == &graph;
    });
    if (iter != m_scenes.end())
    {
        // TODO: error message
        return {};
    }

    // create scene
    auto scenePtr = make_unique_qptr<GraphScene, DirectDeleter>(graph);
    GraphScene* scene = scenePtr.get();

    m_scenes.push_back(std::move(scenePtr));

    // if view has no scene -> set scene
    if (!m_view->nodeScene()) m_view->setScene(*scene);

    return scene;
}
