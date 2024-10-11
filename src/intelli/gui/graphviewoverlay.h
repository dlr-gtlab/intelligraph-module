
#ifndef GT_INTELLI_GRAPHVIEWOVERLAY_H
#define GT_INTELLI_GRAPHVIEWOVERLAY_H

#include <QHBoxLayout>

#include <QPointer>

class QMenu;
class QMenuBar;
class QPushButton;

namespace intelli
{

class GraphScene;
class GraphSceneSelector;
class GraphView;

class GraphViewOverlay : public QHBoxLayout
{
    Q_OBJECT

public:

    GraphViewOverlay(GraphView& view);

    void onSceneRegistered(GraphScene& scene);

signals:

    void gridChanged(QPrivateSignal);

    void connectionShapeChanged(QPrivateSignal);

    void autoEvaluationChanged(QPrivateSignal);

private:

    struct Impl;

    QPointer<GraphView> m_view;

    QMenuBar* m_menuBar = nullptr;
    QMenu* m_sceneMenu = nullptr;
    QMenu* m_editMenu = nullptr;

    QPushButton* m_startAutoEvalBtn = nullptr;
    QPushButton* m_stopAutoEvalBtn = nullptr;
    QPushButton* m_snapToGridBtn = nullptr;

    GraphSceneSelector* m_sceneSelector = nullptr;
};

GraphViewOverlay* makeOverlay(GraphView& view);

} // namespace intelli

#endif // GT_INTELLI_GRAPHVIEWOVERLAY_H
