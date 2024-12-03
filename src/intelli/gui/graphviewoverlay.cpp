/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/graphviewoverlay.h>

#include <intelli/graph.h>
#include <intelli/graphexecmodel.h>
#include <intelli/gui/graphview.h>
#include <intelli/gui/graphscene.h>
#include <intelli/gui/graphsceneselector.h>

#include <gt_objectuiaction.h>
#include <gt_icons.h>
#include <gt_colors.h>
#include <gt_guiutilities.h>

#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSpacerItem>

using namespace intelli;

GraphViewOverlay::GraphViewOverlay(GraphView& view) :
    QHBoxLayout(&view),
    m_view(&view)
{
    m_menuBar = new QMenuBar;

    // helper function to create a separator
    auto const makeSeparator = [](){
        return GtObjectUIAction();
    };

    // helper function to setup scene buttons
    auto setupBtn = [](){
        auto* btn = new QPushButton();
        btn->setVisible(false);
        btn->setEnabled(false);
        auto height = btn->sizeHint().height();
        btn->setFixedSize(QSize(height, height));
        return btn;
    };

    m_snapToGridBtn = setupBtn();
    m_startAutoEvalBtn = setupBtn();
    m_stopAutoEvalBtn = setupBtn();

    /* SLOTS */
    auto changeGridVisibility = [this](auto){
        bool wasVisible = m_view->isGridVisible();
        m_view->showGrid(!wasVisible);
        m_snapToGridBtn->setVisible(!wasVisible);

        auto* scene = m_view->nodeScene();
        if (!scene) return;

        scene->setSnapToGrid(!wasVisible && m_snapToGridBtn->isChecked());
    };

    auto changeConnectionShape = [this](auto){
        if (auto* scene = m_view->nodeScene())
        {
            auto shape = nextConnectionShape(scene->connectionShape());
            scene->setConnectionShape(shape);
        }
    };

    auto changeSnapToGrid = [this](){
        if (auto* scene = m_view->nodeScene())
        {
            scene->setSnapToGrid(m_snapToGridBtn->isChecked());
        }
    };

    auto updateAutoEvaluation = [this](bool autoEvaluate) {
        GraphScene* scene = m_view->nodeScene();
        if (!scene) return;

        auto* model = GraphExecutionModel::accessExecModel(scene->graph());
        if (!model) return;

        autoEvaluate ? (void)model->autoEvaluateGraph(scene->graph()) :
                       (void)model->stopAutoEvaluatingGraph(scene->graph());
    };

    /* VIEW MENU */
    m_viewMenu = m_menuBar->addMenu(tr("View"));
    m_viewMenu->setEnabled(true);

    auto resetScaleAction =
        gt::gui::makeAction(tr("Reset scale"),
                            std::bind(&GraphView::setScale, m_view, 1.0))
            .setIcon(gt::gui::icon::revert());

    auto centerSceneAction =
        gt::gui::makeAction(tr("Center view"),
                            std::bind(&GraphView::centerScene, m_view))
            .setIcon(gt::gui::icon::select());

    auto changeGridAction =
        gt::gui::makeAction(tr("Toggle Grid"), changeGridVisibility)
            .setIcon(gt::gui::icon::grid());

    auto changeConShapeAction =
        gt::gui::makeAction(tr("Toggle Connection Shape"), changeConnectionShape)
            .setIcon(gt::gui::icon::vectorBezier2());

    auto printAction =
        gt::gui::makeAction(tr("Print to PDF"),
                            std::bind(&GraphView::printToPDF, m_view))
            .setIcon(gt::gui::icon::pdf());

    gt::gui::addToMenu(resetScaleAction, *m_viewMenu, nullptr);
    gt::gui::addToMenu(centerSceneAction, *m_viewMenu, nullptr);
    gt::gui::addToMenu(changeConShapeAction, *m_viewMenu, nullptr);
    gt::gui::addToMenu(changeGridAction, *m_viewMenu, nullptr);
    gt::gui::addToMenu(makeSeparator(), *m_viewMenu, nullptr);
    gt::gui::addToMenu(printAction, *m_viewMenu, nullptr);

    /* EDIT MENU */
    m_sceneMenu = m_menuBar->addMenu(tr("Scene"));
    m_sceneMenu->setEnabled(false);

    auto const& viewActions = m_view->actions();
    m_sceneMenu->addActions(viewActions);

    /* AUTO EVAL */
    m_startAutoEvalBtn->setToolTip(tr("Enable automatic graph evaluation"));
    m_startAutoEvalBtn->setIcon(gt::gui::icon::play());
    m_startAutoEvalBtn->setVisible(true);

    m_stopAutoEvalBtn->setToolTip(tr("Stop automatic graph evaluation"));
    m_stopAutoEvalBtn->setIcon(gt::gui::icon::stop());
    m_stopAutoEvalBtn->setVisible(false);

    connect(m_startAutoEvalBtn, &QPushButton::clicked,
        this, std::bind(updateAutoEvaluation, true));
    connect(m_stopAutoEvalBtn, &QPushButton::clicked,
            this, std::bind(updateAutoEvaluation, false));

    /* SNAP TO GRID */
    m_snapToGridBtn->setCheckable(true);
    m_snapToGridBtn->setToolTip(tr("Toggle snap to grid"));
    m_snapToGridBtn->setVisible(m_view->isGridVisible());
    m_snapToGridBtn->setEnabled(false);

    connect(m_snapToGridBtn, &QPushButton::clicked, this, changeSnapToGrid);

    using namespace gt::gui;
    // checked button do not use On/Off Icons, thus we have to update the
    // icon ourselfes (adapted from `GtOutputDock`)
    auto const onCheckedColor = [b = QPointer<QPushButton>(m_snapToGridBtn)](){
        assert (b);
        return b->isChecked() ? color::text() :
                                color::lighten(color::disabled(), 15);
    };
    m_snapToGridBtn->setIcon(colorize(icon::gridSnap(), SvgColorData{ onCheckedColor }));

    /* SCENE-LINK */
    m_sceneSelector = new GraphSceneSelector;

    connect(m_sceneSelector, &GraphSceneSelector::graphClicked,
            this, &GraphViewOverlay::sceneChangeRequested);

    /* OVERLAY */
    setContentsMargins(5, 5, 5, 0);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    int idx = 0;
    insertWidget(idx++, m_menuBar);
    insertWidget(idx++, m_startAutoEvalBtn);
    insertWidget(idx++, m_stopAutoEvalBtn);
    insertWidget(idx++, m_snapToGridBtn);
    insertSpacing(idx , m_snapToGridBtn->width());
    insertWidget(idx++, m_sceneSelector);
    addStretch();

    auto size = m_menuBar->sizeHint();
    size.setWidth(size.width() + 10);
    m_menuBar->setFixedSize(size);

    connect(m_view, &GraphView::sceneChanged,
            this, &GraphViewOverlay::onSceneChanged);

    if (GraphScene* scene = m_view->nodeScene())
    {
        onSceneChanged(scene);
    }
}

GraphViewOverlay*
GraphViewOverlay::make(GraphView& view)
{
    return new GraphViewOverlay(view);
}

void
GraphViewOverlay::onSceneChanged(GraphScene* scene)
{
    assert(m_view);
    if (!scene)
    {
        m_startAutoEvalBtn->setEnabled(false);
        m_stopAutoEvalBtn->setEnabled(false);
        m_snapToGridBtn->setEnabled(false);
        m_snapToGridBtn->setVisible(false);
        m_sceneMenu->setEnabled(false);
        m_viewMenu->setEnabled(false);
        return;
    }

    auto& graph = scene->graph();

    m_sceneSelector->setCurrentGraph(graph);

    m_startAutoEvalBtn->setEnabled(true);
    m_stopAutoEvalBtn->setEnabled(true);
    m_snapToGridBtn->setEnabled(true);
    m_snapToGridBtn->setVisible(m_view->isGridVisible());

    m_viewMenu->setEnabled(true);
    m_sceneMenu->setEnabled(true);

    auto onSnapToGridChanged = [this](){
        if (!m_snapToGridBtn->isVisible()) return;
        if (auto* scene = m_view->nodeScene())
        {
            m_snapToGridBtn->setChecked(scene->snapToGrid());
        }
    };

    auto* model = GraphExecutionModel::accessExecModel(graph);
    if (!model) return;

    auto onAutoEvaluationChanged = [this, model](){
        GraphScene* scene = m_view->nodeScene();
        if (!scene) return;

        bool autoEvaluate = model->isAutoEvaluatingGraph(scene->graph());

        m_startAutoEvalBtn->setVisible(!autoEvaluate);
        m_stopAutoEvalBtn->setVisible(autoEvaluate);
    };

    connect(model, &GraphExecutionModel::autoEvaluationChanged,
            this, onAutoEvaluationChanged);

    connect(scene, &GraphScene::snapToGridChanged,
            m_view, onSnapToGridChanged);

    // update once
    onSnapToGridChanged();
    onAutoEvaluationChanged();
}
