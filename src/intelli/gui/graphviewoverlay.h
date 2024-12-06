/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

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

/**
 * @brief The GraphViewOverlay class. Instanties an overlay widget ontop of a
 * graph view instance. The overlay can be used to control view, scene and exec
 * properties.
 */
class GraphViewOverlay : public QHBoxLayout
{
    Q_OBJECT

public:

    GraphViewOverlay(GraphView& view);
    ~GraphViewOverlay();

    /**
     * @brief Creates an overlay widget ontop the view object. Ownership is
     * taken care of.
     * @param view View to create overlay widget for
     * @return Pointer to overlay widget.
     */
    static GraphViewOverlay* make(GraphView& view);

signals:

    void sceneChangeRequested(QString const& graphUuid);

private:

    QPointer<GraphView> m_view;

    QMenuBar* m_menuBar = nullptr;
    QMenu* m_viewMenu = nullptr;
    QMenu* m_sceneMenu = nullptr;

    QPushButton* m_startAutoEvalBtn = nullptr;
    QPushButton* m_stopAutoEvalBtn = nullptr;
    QPushButton* m_snapToGridBtn = nullptr;

    GraphSceneSelector* m_sceneSelector = nullptr;

private slots:

    void onSceneChanged(GraphScene* scene);
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHVIEWOVERLAY_H
