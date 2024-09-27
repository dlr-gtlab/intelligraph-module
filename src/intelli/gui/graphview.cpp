/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause AND LicenseRef-BSD-3-Clause-Dimitri
 *  SPDX-FileCopyrightText: 2022 Dimitri Pinaev
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/graph.h"
#include "intelli/graphexecmodel.h"
#include "intelli/gui/graphview.h"
#include "intelli/gui/graphscene.h"
#include "intelli/gui/style.h"
#include "intelli/gui/graphics/nodeobject.h"

#include <gt_objectuiaction.h>
#include <gt_icons.h>
#include <gt_colors.h>
#include <gt_guiutilities.h>
#include <gt_application.h>
#include <gt_filedialog.h>
#include <gt_grid.h>
#include <gt_state.h>
#include <gt_statehandler.h>

#include <gt_logging.h>

#include <QCoreApplication>
#include <QWheelEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsWidget>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPrinter>

#include <cmath>

Q_DECLARE_METATYPE(intelli::ConnectionShape)

constexpr int s_major_grid_size = 100;
constexpr int s_minor_grid_size = s_major_grid_size / 10;

using namespace intelli;

struct GraphView::Impl
{

/// attempts to lacate a node at the given scene position
static NodeGraphicsObject* locateNode(QPointF scenePoint,
                                      QGraphicsScene& scene,
                                      QTransform const& viewTransform)
{
    for (auto* item : scene.items(scenePoint,
                                  Qt::IntersectsItemShape,
                                  Qt::DescendingOrder,
                                  viewTransform))
    {
        if (auto* node = qgraphicsitem_cast<NodeGraphicsObject*>(item))
        {
            return node;
        }
    }
    return nullptr;
}

struct GridStateChanged
{
    void operator()(QVariant const& enable)
    {
        assert(view);
        assert(view->m_snapToGridBtn);

        auto* btn = view->m_snapToGridBtn;
        auto* g = view->grid();
        auto* s = view->nodeScene();
        if (!g)
        {
            btn->setVisible(false);
            if (s) s->setSnapToGrid(false);
            return;
        }

        bool enabled = enable.toBool();

        view->resetCachedContent();
        g->showGrid(enabled);
        btn->setVisible(enabled);
        if (s) s->setSnapToGrid(enabled && btn->isChecked());
    }

    GraphView* view{};
};

struct GridValueChanged
{
    void operator()()
    {
        assert(gridState);
        // triggers state update
        gridState->setValue(!gridState->getValue().toBool());
    }

    GtState* gridState{};
};

struct SnapToGridStateChanged
{
    void operator()(QVariant const& enable)
    {
        assert(view);
        assert(view->m_snapToGridBtn);

        bool enabled = enable.toBool();

        auto* btn = view->m_snapToGridBtn;
        btn->setChecked(enabled);

        auto* s = view->nodeScene();
        if (s) s->setSnapToGrid(enabled);
    }

    GraphView* view{};
};

struct SnapToGridValueChanged
{
    void operator()()
    {
        assert(snapToGridState);
        // triggers state update
        snapToGridState->setValue(!snapToGridState->getValue().toBool());
    }

    GtState* snapToGridState{};
};

struct ConnectionShapeStateChanged
{
    void operator()(QVariant const& shape)
    {
        assert(view);
        auto scene = view->nodeScene();
        if (scene) scene->setConnectionShape(shape.value<ConnectionShape>());
    }

    GraphView* view{};
};

struct ConnectionShapeValueChanged
{
    void operator()()
    {
        assert(conShapeState);
        auto value = conShapeState->getValue().value<ConnectionShape>();
        switch (value)
        {
        case ConnectionShape::Cubic:
            value = ConnectionShape::Rectangle;
            break;
        case ConnectionShape::Rectangle:
            value = ConnectionShape::Straight;
            break;
        case ConnectionShape::Straight:
            value = ConnectionShape::Cubic;
        }
        conShapeState->setValue(QVariant::fromValue(value));
    }

    GtState* conShapeState{};
};

/// helper function to create a state and update it accordingly when signals are
/// fired.
template<typename StateChangedSlot,
         typename ValueChangedSlot,
         typename Value,
         typename Signal,
         typename Sender = GraphView>
static GtState* setupState(GraphView& view,
                           GtObject& guardian,
                           Graph& graph,
                           QString const& stateId,
                           Value defaultValue,
                           Sender* sender,
                           Signal signal)
{
    /// grid change state
    auto* state = gtStateHandler->initializeState(
        // group id
        GT_CLASSNAME(GraphView),
        // state id
        stateId,
        // entry for this graph
        graph.uuid() + QChar(';') + stateId.toLower().replace(' ', '_'),
        // default value
        defaultValue,
        // guardian object
        &guardian);

    connect(state, qOverload<QVariant const&>(&GtState::valueChanged),
            &view, StateChangedSlot{&view});
    connect(sender, signal,
            state, ValueChangedSlot{state});

    // trigger grid update
    emit state->valueChanged(state->getValue());
    return state;
}

}; // Impl

GraphView::GraphView(QWidget* parent) :
    GtGraphicsView(nullptr, parent)
{
    setDragMode(QGraphicsView::ScrollHandDrag);
    setRenderHint(QPainter::Antialiasing);

    auto& style = style::currentStyle().view;

    setBackgroundBrush(style.background);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    setCacheMode(QGraphicsView::CacheBackground);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);

    // disable scale range
    setScaleRange(0, 0);

    // Sets the scene rect to its maximum possible ranges to avoid auto scene range
    // re-calculation when expanding the all QGraphicsItems common rect.
    constexpr int maxSize = 32767;
    setSceneRect(-maxSize, -maxSize, (maxSize * 2), (maxSize * 2));

    /* GRID */
    auto* grid = new GtGrid(*this);
    setGrid(grid);
    grid->setHorizontalGridLineColor(style.gridline);
    grid->setVerticalGridLineColor(style.gridline);
    grid->setShowAxis(false);
    // controls minor and major lines
    grid->setGridHeight(s_major_grid_size);
    grid->setGridWidth(s_major_grid_size);

    /* MENU BAR */
    auto* menuBar = new QMenuBar;

    auto const makeSeparator = [](){
        return GtObjectUIAction();
    };

    /* SCENE MENU */
    m_sceneMenu = menuBar->addMenu(tr("Scene"));
    m_sceneMenu->setEnabled(false);

    auto resetScaleAction =
        gt::gui::makeAction(tr("Reset scale"), std::bind(&GraphView::setScale, this, 1))
            .setIcon(gt::gui::icon::revert());

    auto centerSceneAction =
        gt::gui::makeAction(tr("Center scene"), std::bind(&GraphView::centerScene, this))
            .setIcon(gt::gui::icon::select());

    auto changeGrid =
        gt::gui::makeAction(tr("Toggle Grid"), std::bind(&GraphView::gridChanged, this, QPrivateSignal()))
            .setIcon(gt::gui::icon::grid());

    auto changeConShape =
        gt::gui::makeAction(tr("Toggle Connection Shape"), std::bind(&GraphView::connectionShapeChanged, this, QPrivateSignal()))
            .setIcon(gt::gui::icon::vectorBezier2());

    auto print =
        gt::gui::makeAction(tr("Print to PDF"), std::bind(&GraphView::printPDF, this))
              .setIcon(gt::gui::icon::pdf());

    gt::gui::addToMenu(resetScaleAction, *m_sceneMenu, nullptr);
    gt::gui::addToMenu(centerSceneAction, *m_sceneMenu, nullptr);
    gt::gui::addToMenu(changeConShape, *m_sceneMenu, nullptr);
    gt::gui::addToMenu(changeGrid, *m_sceneMenu, nullptr);
    gt::gui::addToMenu(makeSeparator(), *m_sceneMenu, nullptr);
    gt::gui::addToMenu(print, *m_sceneMenu, nullptr);

    /* EDIT MENU */
    m_editMenu = menuBar->addMenu(tr("Edit"));
    m_editMenu->setEnabled(false);

    /* AUTO EVAL */
    auto setupBtn = [](){
        auto* btn = new QPushButton();
        btn->setVisible(false);
        btn->setEnabled(false);
        auto height = btn->sizeHint().height();
        btn->setFixedSize(QSize(height, height));
        return btn;
    };

    m_startAutoEvalBtn = setupBtn();
    m_startAutoEvalBtn->setVisible(true);
    m_startAutoEvalBtn->setToolTip(tr("Enable automatic graph evaluation"));
    m_startAutoEvalBtn->setIcon(gt::gui::icon::play());

    m_stopAutoEvalBtn = setupBtn();
    m_stopAutoEvalBtn->setToolTip(tr("Stop automatic graph evaluation"));
    m_stopAutoEvalBtn->setIcon(gt::gui::icon::stop());

    m_snapToGridBtn = setupBtn();
    m_snapToGridBtn->setCheckable(true);
    m_snapToGridBtn->setToolTip(tr("Toggle snap to grid"));
    m_snapToGridBtn->setVisible(true);
    m_snapToGridBtn->setEnabled(false);

    using gt::gui::color::lighten;
    using gt::gui::color::disabled;
    using gt::gui::color::text;
    using gt::gui::colorize; // use custom colors for icon

    {
        auto* button = m_snapToGridBtn;
        // checked button do not use On/Off Icons, thus we have to update the
        // icon ourselfes (adapted from `GtOutputDock`)
        auto const updateIconColor = [b = QPointer<QPushButton>(button)](){
            assert (b);
            return b->isChecked() ? text() : lighten(disabled(), 15);
        };
        button->setIcon(colorize(gt::gui::icon::gridSnap(),
                                 gt::gui::SvgColorData{ updateIconColor }));
    }

    /* OVERLAY */
    auto* overlay = new QHBoxLayout(this);
    overlay->setContentsMargins(5, 5, 0, 0);
    overlay->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    overlay->addWidget(menuBar);
    overlay->addWidget(m_startAutoEvalBtn);
    overlay->addWidget(m_stopAutoEvalBtn);
    overlay->addWidget(m_snapToGridBtn);
    overlay->addStretch();

    auto size = menuBar->sizeHint();
    size.setWidth(size.width() + 10);
    menuBar->setFixedSize(size);
}

void
GraphView::setScene(GraphScene& scene)
{
    QGraphicsView::setScene(&scene);
    centerScene();

    auto* guardian = new GtObject();
    guardian->setParent(&scene);

    auto& graph = scene.graph();

    /// grid change state
    auto* gridState =
        Impl::setupState<Impl::GridStateChanged,
                         Impl::GridValueChanged>(
            *this, *guardian, graph, tr("Show Grid"), true,
            this, &GraphView::gridChanged
    );

    /// snap to grid state
    Impl::setupState<Impl::SnapToGridStateChanged,
                     Impl::SnapToGridValueChanged>(
        *this, *guardian, graph, tr("Snap to Grid"), true,
        m_snapToGridBtn, &QPushButton::clicked
    );

    /// connection style state
    Impl::setupState<Impl::ConnectionShapeStateChanged,
                     Impl::ConnectionShapeValueChanged>(
        *this, *guardian, graph, tr("Connection Shape"),
        QVariant::fromValue(ConnectionShape::DefaultShape),
        this, &GraphView::connectionShapeChanged
    );

    /// snap nodes to minor grid
    scene.setGridSize(s_minor_grid_size);
    bool gridEnabled = gridState->getValue().toBool();
    if (!gridEnabled) scene.setSnapToGrid(false);

    m_sceneMenu->setEnabled(true);

    m_editMenu->clear();
    m_editMenu->setEnabled(true);

    // setup actions
    auto* alignAction = m_editMenu->addAction(tr("Align Nodes to Grid"));
    alignAction->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
    alignAction->setIcon(gt::gui::icon::gridSnap());
    alignAction->setEnabled(gridEnabled);
    connect(alignAction, &QAction::triggered,
            &scene, qOverload<>(&GraphScene::alignObjectsToGrid),
            Qt::UniqueConnection);
    // enable/disable if grid is toggled
    connect(this, &GraphView::gridChanged,
            alignAction, [gridState, alignAction](){
        alignAction->setEnabled(gridState->getValue().toBool());
    });

    auto* copyAction = m_editMenu->addAction(tr("Copy Selection"));
    copyAction->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
    copyAction->setShortcut(gtApp->getShortCutSequence("copy"));
    copyAction->setIcon(gt::gui::icon::copy());
    connect(copyAction, &QAction::triggered,
            &scene, &GraphScene::copySelectedObjects,
            Qt::UniqueConnection);

    auto* pasteAction = m_editMenu->addAction(tr("Paste Selection"));
    pasteAction->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
    pasteAction->setShortcut(gtApp->getShortCutSequence("paste"));
    pasteAction->setIcon(gt::gui::icon::paste());
    connect(pasteAction, &QAction::triggered,
            &scene, &GraphScene::pasteObjects,
            Qt::UniqueConnection);

    auto* duplicateAction = m_editMenu->addAction(tr("Duplicate Selection"));
    duplicateAction->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
    duplicateAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    duplicateAction->setIcon(gt::gui::icon::duplicate());
    connect(duplicateAction, &QAction::triggered,
            &scene, &GraphScene::duplicateSelectedObjects,
            Qt::UniqueConnection);

    auto* deleteAction = m_editMenu->addAction(tr("Delete Selection"));
    deleteAction->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
    deleteAction->setShortcut(gtApp->getShortCutSequence("delete"));
    deleteAction->setIcon(gt::gui::icon::delete_());
    connect(deleteAction, &QAction::triggered,
            &scene, &GraphScene::deleteSelectedObjects,
            Qt::UniqueConnection);

    auto* clearSelection = new QAction(tr("Clear Selection"), this);
    clearSelection->setShortcut(Qt::Key_Escape);

    connect(clearSelection, &QAction::triggered,
            &scene, &QGraphicsScene::clearSelection,
            Qt::UniqueConnection);

    auto updateAutoEvalBtns = [this, &graph](){
        if (auto* s = nodeScene())
        {
            if (&s->graph() != &graph) return;

            auto* model = GraphExecutionModel::accessExecModel(graph);
            if (!model || &(model->graph()) != &graph) return;

            bool isAutoEvaluating = model->isAutoEvaluatingGraph();
            m_startAutoEvalBtn->setVisible(!isAutoEvaluating);
            m_stopAutoEvalBtn->setVisible(isAutoEvaluating);
        }
    };

    m_startAutoEvalBtn->setEnabled(true);
    m_stopAutoEvalBtn->setEnabled(true);
    m_snapToGridBtn->setEnabled(true);

    updateAutoEvalBtns();

    connect(&graph, &Graph::isActiveChanged, this, updateAutoEvalBtns);

    connect(m_startAutoEvalBtn, &QPushButton::clicked,
            this, [this](){
        if (auto* scene = nodeScene())
        if (auto* model = GraphExecutionModel::accessExecModel(scene->graph()))
        {
            model->autoEvaluateGraph();
        }
    });
    connect(m_stopAutoEvalBtn, &QPushButton::clicked,
            this, [this](){
        if (auto* scene = nodeScene())
        if (auto* model = GraphExecutionModel::accessExecModel(scene->graph()))
        {
            model->stopAutoEvaluatingGraph();
        }
    });
    connect(m_snapToGridBtn, &QPushButton::clicked,
            this, [this](){
        if (auto* s = nodeScene())
        {
            s->setSnapToGrid(m_snapToGridBtn->isChecked());
        }
    });

    QAction* separtor = new QAction;
    separtor->setSeparator(true);

    addAction(alignAction);
    addAction(separtor);
    addAction(copyAction);
    addAction(pasteAction);
    addAction(duplicateAction);
    addAction(deleteAction);
    addAction(clearSelection);
}

void
GraphView::setScaleRange(double minimum, double maximum)
{
    if (maximum < minimum) std::swap(minimum, maximum);
    minimum = std::max(0.0, minimum);
    maximum = std::max(0.0, maximum);

    m_scaleRange = {minimum, maximum};

    setScale(transform().m11());
}

void
GraphView::setScaleRange(ScaleRange range)
{
    setScaleRange(range.minimum, range.maximum);
}

double
GraphView::scale() const
{
    return transform().m11();
}

void
GraphView::scaleUp()
{
    constexpr double step = 1.1;
    constexpr double factor = step;

    if (m_scaleRange.maximum > 0)
    {
        QTransform t = transform();
        t.scale(factor, factor);
        if (t.m11() >= m_scaleRange.maximum) return setScale(t.m11());
    }

    QGraphicsView::scale(factor, factor);
    emit scaleChanged(transform().m11());
}

void
GraphView::scaleDown()
{
    constexpr double step = 1.1;
    constexpr double factor = 1 / step;

    if (m_scaleRange.minimum > 0)
    {
        QTransform t = transform();
        t.scale(factor, factor);
        if (t.m11() <= m_scaleRange.minimum) return setScale(t.m11());
    }

    QGraphicsView::scale(factor, factor);
    emit scaleChanged(transform().m11());
}

void
GraphView::setScale(double scale)
{
    if (m_scaleRange.minimum > 0.0)
    {
        scale = std::max(m_scaleRange.minimum, scale);
    }
    if (m_scaleRange.maximum > 0.0)
    {
        scale = std::min(m_scaleRange.maximum, scale);
    }

    if (scale <= 0) return;

    if (scale == transform().m11()) return;

    QTransform matrix;
    matrix.scale(scale, scale);
    setTransform(matrix, false);

    emit scaleChanged(scale);
}

void
GraphView::printPDF()
{
    QString filePath =
        GtFileDialog::getSaveFileName(parentWidget(),
                                      tr("Choose File"),
                                      QString(),
                                      tr("PDF files (*.pdf)"),
                                      QStringLiteral("snapshot.pdf"));

    if (filePath.isEmpty())
    {
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setPageSize(QPrinter::A4);
    printer.setPageOrientation(QPageLayout::Landscape);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);

    QPainter p;

    if (!p.begin(&printer))
    {
        gtError() << tr("Error while initializing print!");
        return;
    }

    scene()->render(&p);
    p.end();
}

void
GraphView::centerScene()
{
    if (scene())
    {
        QRectF sceneRect = scene()->sceneRect();

        if (sceneRect.width()  > rect().width() ||
            sceneRect.height() > rect().height())
        {
            fitInView(sceneRect, Qt::KeepAspectRatio);
        }

        centerOn(sceneRect.center());
    }
}

void
GraphView::contextMenuEvent(QContextMenuEvent* event)
{
    if (itemAt(event->pos()))
    {
        return QGraphicsView::contextMenuEvent(event);
    }

    auto const scenePos = mapToScene(mapFromGlobal(QCursor::pos()));

    if (QMenu* menu = nodeScene()->createSceneMenu(scenePos))
    {
        menu->exec(event->globalPos());
    }
}

void
GraphView::wheelEvent(QWheelEvent* event)
{
    if (!(event->modifiers() & Qt::Modifier::CTRL))
    if (auto* s = scene())
    {
        auto pos = event->position().toPoint();
        auto* node = Impl::locateNode(mapToScene(pos), *s, transform());

        if (node)
        if (auto* w = node->centralWidget())
        {
            QPolygon bounding = mapFromScene(w->sceneBoundingRect());

            // forward event to widget
            if (bounding.containsPoint(pos, Qt::OddEvenFill))
            {
                QGraphicsSceneWheelEvent wheelEvent(QEvent::GraphicsSceneWheel);
                wheelEvent.setWidget(viewport());
                wheelEvent.setScenePos(mapToScene(event->position().toPoint()));
                wheelEvent.setScreenPos(event->globalPosition().toPoint());
                wheelEvent.setButtons(event->buttons());
                wheelEvent.setModifiers(event->modifiers());
                wheelEvent.setDelta(event->delta());
                wheelEvent.setOrientation(event->orientation());
                wheelEvent.setAccepted(false);
                QCoreApplication::sendEvent(s, &wheelEvent);
                return;
            }
        }
    }

// (refactored)
// SPDX-SnippetBegin
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Dimitri
// SPDX-SnippetCopyrightText: 2022 Dimitri Pinaev
    QPoint delta = event->angleDelta();

    if (delta.y() == 0)
    {
        return event->ignore();
    }

    double const d = delta.y() / std::abs(delta.y());

    (d > 0.0) ? scaleUp() : scaleDown();
// SPDX-SnippetEnd
}

void
GraphView::keyPressEvent(QKeyEvent* event)
{
// SPDX-SnippetBegin
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Dimitri
// SPDX-SnippetCopyrightText: 2022 Dimitri Pinaev
    switch (event->key())
    {
    case Qt::Key_Shift:
        setDragMode(QGraphicsView::RubberBandDrag);
        break;
    default:
        break;
    }

    QGraphicsView::keyPressEvent(event);
// SPDX-SnippetEnd
}

void
GraphView::keyReleaseEvent(QKeyEvent* event)
{
// SPDX-SnippetBegin
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Dimitri
// SPDX-SnippetCopyrightText: 2022 Dimitri Pinaev
    switch (event->key())
    {
    case Qt::Key_Shift:
        setDragMode(QGraphicsView::ScrollHandDrag);
        break;
    default:
        break;
    }
    QGraphicsView::keyReleaseEvent(event);
// SPDX-SnippetEnd
}

void
GraphView::mousePressEvent(QMouseEvent* event)
{
// SPDX-SnippetBegin
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Dimitri
// SPDX-SnippetCopyrightText: 2022 Dimitri Pinaev
    QGraphicsView::mousePressEvent(event);
    if (event->button() & Qt::LeftButton)
    {
        m_panPosition = mapToScene(event->pos());
    }
// SPDX-SnippetEnd
}

void
GraphView::mouseMoveEvent(QMouseEvent* event)
{
// SPDX-SnippetBegin
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Dimitri
// SPDX-SnippetCopyrightText: 2022 Dimitri Pinaev
    QGraphicsView::mouseMoveEvent(event);
    if (scene() && !scene()->mouseGrabberItem() && event->buttons() & Qt::LeftButton)
    {
        // Make sure shift is not being pressed
        if (!(event->modifiers() & Qt::ShiftModifier))
        {
            QPointF difference = m_panPosition - mapToScene(event->pos());
            setSceneRect(sceneRect().translated(difference.x(), difference.y()));
        }
    }
// SPDX-SnippetEnd
}

GraphScene*
GraphView::nodeScene()
{
    return qobject_cast<GraphScene*>(scene());
}
