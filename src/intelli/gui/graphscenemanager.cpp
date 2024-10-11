
#include <intelli/gui/graphscenemanager.h>

#include <intelli/graph.h>
#include <intelli/graphexecmodel.h>
#include <intelli/gui/graphview.h>
#include <intelli/gui/graphscene.h>

using namespace intelli;

GraphSceneManager::GraphSceneManager(GraphView& view) :
    QObject((QObject*)&view),
    m_view(&view)
{

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
    auto scenePtr = make_q_unique<GraphScene, DirectDeleter>(graph);
    auto* scene = scenePtr.get();

    m_scenes.push_back(std::move(scenePtr));

    // if view has no scene -> set scene
    if (!m_view->nodeScene()) m_view->setScene(*scene);

    // if no scene exists -> setup exec model
    if (m_scenes.size() == 1)
    {
        auto* root = &graph;
        assert(root);

        auto* model = GraphExecutionModel::accessExecModel(*root);
        if (!model)
        {
            model = new GraphExecutionModel(*root);
        }
        Q_UNUSED(model);
    }

    return scene;
}
