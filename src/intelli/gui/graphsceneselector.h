#ifndef GT_INTELLI_GRAPHSCENESELECTOR_H
#define GT_INTELLI_GRAPHSCENESELECTOR_H

#include <intelli/globals.h>

#include <QWidget>
#include <QPointer>

class QLabel;

namespace intelli
{

class Graph;
class GraphSceneSelector : public QWidget
{
    Q_OBJECT

public:

    GraphSceneSelector(QWidget* parent = nullptr);

    void setCurrentGraph(Graph& graph);

public slots:

    void clear();

    void refresh();

signals:

    void graphSelected(NodeUuid const& graphUuid);

private:

    QPointer<Graph> m_currentGraph;
    QLabel* m_scenePath;
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHSCENESELECTOR_H
