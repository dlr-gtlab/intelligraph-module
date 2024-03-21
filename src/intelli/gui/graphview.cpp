/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/gui/graphview.h"
#include "intelli/gui/graphscene.h"
#include "intelli/gui/style.h"
#include "intelli/graph.h"
#include "intelli/graphexecmodel.h"

#include <gt_objectuiaction.h>
#include <gt_icons.h>
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

using namespace intelli;

struct GraphView::Impl
{
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
};

GraphView::GraphView(QWidget* parent) :
    GtGraphicsView(nullptr, parent)
{
    setDragMode(QGraphicsView::ScrollHandDrag);
    setRenderHint(QPainter::Antialiasing);

    setBackgroundBrush(style::viewBackground());

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    setCacheMode(QGraphicsView::CacheBackground);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);

    setScaleRange(0.3, 2);

    // Sets the scene rect to its maximum possible ranges to avoid auto scene range
    // re-calculation when expanding the all QGraphicsItems common rect.
    int maxSize = 32767;
    setSceneRect(-maxSize, -maxSize, (maxSize * 2), (maxSize * 2));

    /* GRID */
    auto* grid = new GtGrid(*this);
    setGrid(grid);
    grid->setShowAxis(false);
    grid->setGridHeight(15);
    grid->setGridWidth(15);

    /* MENU BAR */
    auto* menuBar = new QMenuBar;

    /* SCENE MENU */
    m_sceneMenu = menuBar->addMenu(tr("Scene"));
    m_sceneMenu->setEnabled(false);

    auto resetScaleAction =
        gt::gui::makeAction(tr("Reset scale"), std::bind(&GraphView::setScale, this, 1))
              .setIcon(gt::gui::icon::revert());

    auto changeGrid =
        gt::gui::makeAction(tr("Toogle Grid"), std::bind(&GraphView::gridChanged, this, QPrivateSignal()))
            .setIcon(gt::gui::icon::grid());

    auto print =
        gt::gui::makeAction(tr("Print to PDF"), std::bind(&GraphView::printPDF, this))
              .setIcon(gt::gui::icon::pdf());

    gt::gui::addToMenu(resetScaleAction, *m_sceneMenu, nullptr);
    gt::gui::addToMenu(changeGrid, *m_sceneMenu, nullptr);
    gt::gui::addToMenu(print, *m_sceneMenu, nullptr);

    /* EDIT MENU */
    m_editMenu = menuBar->addMenu(tr("Edit"));
    m_editMenu->setEnabled(false);

    /* AUTO EVAL */
    auto setupEvalBtn = [](){
        auto* btn = new QPushButton();
        btn->setVisible(false);
        btn->setEnabled(false);
        auto height = btn->sizeHint().height();
        btn->setFixedSize(QSize(height * 1.5, height));
        return btn;
    };

    m_startAutoEvalBtn = setupEvalBtn();
    m_startAutoEvalBtn->setVisible(true);
    m_startAutoEvalBtn->setToolTip(tr("Enable automatic graph evaluation"));
    m_startAutoEvalBtn->setIcon(gt::gui::icon::play());

    m_stopAutoEvalBtn = setupEvalBtn();
    m_stopAutoEvalBtn->setToolTip(tr("Stop automatic graph evaluation"));
    m_stopAutoEvalBtn->setIcon(gt::gui::icon::stop());

    /* OVERLAY */
    auto* overlay = new QHBoxLayout(this);
    overlay->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    overlay->addWidget(menuBar);
    overlay->addWidget(m_startAutoEvalBtn);
    overlay->addWidget(m_stopAutoEvalBtn);
    overlay->addStretch();

    auto size = menuBar->sizeHint();
    size.setWidth(size.width() + 10);
    menuBar->setFixedSize(size);
}

void
GraphView::setScene(GraphScene& scene)
{
    QGraphicsView::setScene(&scene);

    auto guardian = new GtObject();
    guardian->setParent(&scene);

    auto& graph = scene.graph();

    auto* state = gtStateHandler->initializeState(GT_CLASSNAME(GraphView),
                                    tr("Show Grid"),
                                    graph.uuid() + QStringLiteral(";show_grid"),
                                    true, guardian);

    connect(state, qOverload<QVariant const&>(&GtState::valueChanged),
            this, [this](QVariant const& enable){
        resetCachedContent();

        if (auto* g = grid())
        {
            g->showGrid(enable.toBool());
        }
    });
    connect(this, &GraphView::gridChanged, state, [state](){
        state->setValue(!state->getValue().toBool());
    });

    // trigger grid update
    emit state->valueChanged(state->getValue());

    m_sceneMenu->setEnabled(true);

    m_editMenu->clear();
    m_editMenu->setEnabled(true);

    // setup actions
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
        auto* exec = graph.executionModel();
        bool isAutoEvaluating = exec && exec->isAutoEvaluating();

        m_startAutoEvalBtn->setVisible(!isAutoEvaluating);
        m_stopAutoEvalBtn->setVisible(isAutoEvaluating);
    };

    m_startAutoEvalBtn->setEnabled(true);
    m_stopAutoEvalBtn->setEnabled(true);

    updateAutoEvalBtns();

    connect(&graph, &Graph::isActiveChanged, this, updateAutoEvalBtns);

    connect(m_startAutoEvalBtn, &QPushButton::clicked, &graph, [&graph](){
        graph.setActive(true);
    });
    connect(m_stopAutoEvalBtn, &QPushButton::clicked, &graph, [&graph](){
        graph.setActive(false);
    });

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
    scale = std::max(m_scaleRange.minimum, std::min(m_scaleRange.maximum, scale));

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

    QPoint delta = event->angleDelta();

    if (delta.y() == 0)
    {
        return event->ignore();
    }

    double const d = delta.y() / std::abs(delta.y());

    (d > 0.0) ? scaleUp() : scaleDown();
}

void
GraphView::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Shift:
        setDragMode(QGraphicsView::RubberBandDrag);
        break;
    default:
        break;
    }

    QGraphicsView::keyPressEvent(event);
}

void
GraphView::keyReleaseEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Shift:
        setDragMode(QGraphicsView::ScrollHandDrag);
        break;
    default:
        break;
    }
    QGraphicsView::keyReleaseEvent(event);
}

void
GraphView::mousePressEvent(QMouseEvent* event)
{
    QGraphicsView::mousePressEvent(event);
    if (event->button() & Qt::LeftButton)
    {
        m_panPosition = mapToScene(event->pos());
    }
}

void
GraphView::mouseMoveEvent(QMouseEvent* event)
{
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
}

GraphScene*
GraphView::nodeScene()
{
    return qobject_cast<GraphScene*>(scene());
}
