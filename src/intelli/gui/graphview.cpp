/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/gui/graphview.h"
#include "intelli/gui/graphscene.h"

#include "gt_objectuiaction.h"
#include "gt_icons.h"
#include "gt_guiutilities.h"
#include "gt_application.h"
#include "gt_grid.h"
#include "gt_filedialog.h"


#include <gt_logging.h>

#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/internal/locateNode.hpp>
#include <QtNodes/internal/NodeGraphicsObject.hpp>

#include <QCoreApplication>
#include <QWheelEvent>
#include <QGraphicsSceneWheelEvent>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QPrinter>


#include <cmath>

using namespace intelli;

GraphView::GraphView(QWidget* parent) :
    GtGraphicsView(nullptr, parent)
{
    setDragMode(QGraphicsView::ScrollHandDrag);
    setRenderHint(QPainter::Antialiasing);

    auto const &flowViewStyle = QtNodes::StyleCollection::flowViewStyle();
    setBackgroundBrush(flowViewStyle.BackgroundColor);

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

    /* MENU BAR */
    auto* menuBar = new QMenuBar;

    /* SCENE MENU */
    QMenu* sceneMenu = menuBar->addMenu(tr("Scene"));
    m_sceneMenu = sceneMenu;
    m_sceneMenu->setEnabled(false);

    auto resetScaleAction = gt::gui::makeAction(tr("Reset scale"), [this](GtObject*){
        setScale(1);
    });
    resetScaleAction.setIcon(gt::gui::icon::revert());

    auto changeGrid = gt::gui::makeAction(tr("Change Grid"), [this](GtObject*){   
        emit changeGridTriggered();
    });
    changeGrid.setIcon(gt::gui::icon::grid());

    auto print = gt::gui::makeAction(tr("Print to pdf"), [this](GtObject*){
        printPDF();
    });
    print.setIcon(gt::gui::icon::pdf());

    gt::gui::addToMenu(resetScaleAction, *sceneMenu, nullptr);
    gt::gui::addToMenu(changeGrid, *sceneMenu, nullptr);
    gt::gui::addToMenu(print, *sceneMenu, nullptr);

    /* EDIT MENU */
    QMenu* editMenu = menuBar->addMenu(tr("Edit"));
    m_editMenu = editMenu;
    m_editMenu->setEnabled(false);

    /* OVERLAY */
    auto* overlay = new QVBoxLayout(this);
    overlay->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    overlay->addWidget(menuBar);

    auto size = menuBar->sizeHint();
    size.setWidth(size.width() + 10);
    menuBar->setFixedSize(size);
}

void
GraphView::setScene(GraphScene& scene)
{
    QGraphicsView::setScene(&scene);

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

    auto* autoEvaluate = m_sceneMenu->addAction(tr("Enable auto evaluation"));
    autoEvaluate->setIcon(gt::gui::icon::play());
    auto* stopAutoEvaluate = m_sceneMenu->addAction(tr("Disabled auto evaluation"));
    stopAutoEvaluate->setIcon(gt::gui::icon::stop());

    addAction(autoEvaluate);
    addAction(stopAutoEvaluate);

    connect(m_sceneMenu, &QMenu::aboutToShow,
            this, [&scene, autoEvaluate, stopAutoEvaluate](){
        bool isAutoEvaluating = scene.isAutoEvaluating();
        autoEvaluate->setVisible(!isAutoEvaluating);
        stopAutoEvaluate->setVisible(isAutoEvaluating);
    });

    connect(autoEvaluate, &QAction::triggered, &scene, [=](){
        if (auto* scene = nodeScene())
        {
            scene->autoEvaluate(true);
        }
    });
    connect(stopAutoEvaluate, &QAction::triggered, &scene, [=](){
        if (auto* scene = nodeScene())
        {
            scene->autoEvaluate(false);
        }
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
    gtTrace() << __FUNCTION__;
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
        gtError() << tr("Error while initialize print!");
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
        auto* node = QtNodes::locateNodeAt(mapToScene(pos), *s, transform());

        if (node && node->centralWidget())
        {
            auto* w = node->centralWidget();

            auto bounding = mapFromScene(w->sceneBoundingRect());

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

    if (delta.y() == 0) {
        event->ignore();
        return;
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
