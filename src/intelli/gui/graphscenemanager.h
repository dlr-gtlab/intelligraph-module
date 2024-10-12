
#ifndef GT_INTELLI_GRAPHSCENEMANAGER_H
#define GT_INTELLI_GRAPHSCENEMANAGER_H

#include <intelli/memory.h>

#include <QObject>
#include <QPointer>

namespace intelli
{

class Graph;
class GraphScene;
class GraphView;

class GraphSceneManager : public QObject
{
    Q_OBJECT

public:

    GraphSceneManager(GraphView& view);

    GraphScene* createScene(Graph& graph);

private:

    QPointer<GraphView> m_view;

    std::vector<unique_qptr<GraphScene, DirectDeleter>> m_scenes;
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHSCENEMANAGER_H
