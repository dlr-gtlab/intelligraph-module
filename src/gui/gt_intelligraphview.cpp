/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphview.h"

#include "gt_command.h"
#include "gt_objectuiaction.h"
#include "gt_customactionmenu.h"
#include "gt_filedialog.h"
#include "gt_icons.h"
#include "gt_utilities.h"
#include "gt_application.h"

#include <gt_logging.h>

#include <QtNodes/BasicGraphicsScene>
#include <QtNodes/StyleCollection>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/internal/locateNode.hpp>
#include <QtNodes/internal/NodeGraphicsObject.hpp>

#include <QFileInfo>
#include <QFile>
#include <QClipboard>
#include <QApplication>
#include <QWheelEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsProxyWidget>
#include <QMenuBar>
#include <QJsonDocument>
#include <QVBoxLayout>

GtIntelliGraphView::GtIntelliGraphView(QWidget *parent) :
    QGraphicsView(parent)
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

    GtObjectUIAction saveAction(tr("Save"), [this](GtObject*){
        saveToJson();
    });
    saveAction.setIcon(gt::gui::icon::save());

    GtObjectUIAction loadAction(tr("Load"), [this](GtObject*){
        loadFromJson();
    });
    loadAction.setIcon(gt::gui::icon::import());

    GtObjectUIAction printGraphAction(tr("Copy to clipboard"), [this](GtObject*){
        if (auto model = graphModel())
        {
            QJsonDocument doc(model->save());
            QApplication::clipboard()->setText(doc.toJson(QJsonDocument::Indented));
        }
    });
    printGraphAction.setIcon(gt::gui::icon::copy());

    GtObjectUIAction resetScaleAction(tr("Reset scale"), [this](GtObject*){
        setScale(1);
    });
    resetScaleAction.setIcon(gt::gui::icon::revert());

    new GtCustomActionMenu({resetScaleAction}, nullptr, nullptr, sceneMenu);

    m_sceneMenu->addSeparator();

    new GtCustomActionMenu({saveAction, loadAction, printGraphAction}, nullptr, nullptr, sceneMenu);

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
GtIntelliGraphView::setScene(GtIntelliGraphScene& scene)
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
            &scene, &GtIntelliGraphScene::copySelectedObjects,
            Qt::UniqueConnection);

    auto* pasteAction = m_editMenu->addAction(tr("Paste Selection"));
    pasteAction->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
    pasteAction->setShortcut(gtApp->getShortCutSequence("paste"));
    pasteAction->setIcon(gt::gui::icon::paste());
    connect(pasteAction, &QAction::triggered,
            &scene, &GtIntelliGraphScene::pasteObjects,
            Qt::UniqueConnection);

    auto* duplicateAction = m_editMenu->addAction(tr("Duplicate Selection"));
    duplicateAction->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
    duplicateAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    duplicateAction->setIcon(gt::gui::icon::duplicate());
    connect(duplicateAction, &QAction::triggered,
            &scene, &GtIntelliGraphScene::duplicateSelectedObjects,
            Qt::UniqueConnection);

    auto* deleteAction = m_editMenu->addAction(tr("Delete Selection"));
    deleteAction->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
    deleteAction->setShortcut(gtApp->getShortCutSequence("delete"));
    deleteAction->setIcon(gt::gui::icon::delete_());
    connect(deleteAction, &QAction::triggered,
            &scene, &GtIntelliGraphScene::deleteSelectedObjects,
            Qt::UniqueConnection);

    auto* clearSelection = new QAction(tr("Clear Selection"), this);
    clearSelection->setShortcut(Qt::Key_Escape);

    connect(clearSelection, &QAction::triggered,
            &scene, &QGraphicsScene::clearSelection,
            Qt::UniqueConnection);

    addAction(copyAction);
    addAction(pasteAction);
    addAction(duplicateAction);
    addAction(deleteAction);
    addAction(clearSelection);
}

void
GtIntelliGraphView::setScaleRange(double minimum, double maximum)
{
    if (maximum < minimum) std::swap(minimum, maximum);
    minimum = std::max(0.0, minimum);
    maximum = std::max(0.0, maximum);

    m_scaleRange = {minimum, maximum};

    setScale(transform().m11());
}

void
GtIntelliGraphView::setScaleRange(ScaleRange range)
{
    setScaleRange(range.minimum, range.maximum);
}

double
GtIntelliGraphView::scale() const
{
    return transform().m11();
}

void
GtIntelliGraphView::scaleUp()
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
GtIntelliGraphView::scaleDown()
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
GtIntelliGraphView::setScale(double scale)
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
GtIntelliGraphView::centerScene()
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
GtIntelliGraphView::contextMenuEvent(QContextMenuEvent* event)
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
GtIntelliGraphView::wheelEvent(QWheelEvent* event)
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
GtIntelliGraphView::keyPressEvent(QKeyEvent* event)
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
GtIntelliGraphView::keyReleaseEvent(QKeyEvent* event)
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
GtIntelliGraphView::mousePressEvent(QMouseEvent* event)
{
    QGraphicsView::mousePressEvent(event);
    if (event->button() & Qt::LeftButton)
    {
        m_panPosition = mapToScene(event->pos());
    }
}

void
GtIntelliGraphView::mouseMoveEvent(QMouseEvent* event)
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

void
GtIntelliGraphView::drawBackground(QPainter* painter, const QRectF& r)
{
    QGraphicsView::drawBackground(painter, r);

    if (!scene()) return;

    auto drawGrid = [&](double gridStep) {
        QRect windowRect = rect();
        QPointF tl = mapToScene(windowRect.topLeft());
        QPointF br = mapToScene(windowRect.bottomRight());

        double left = std::floor(tl.x() / gridStep - 0.5);
        double right = std::floor(br.x() / gridStep + 1.0);
        double bottom = std::floor(tl.y() / gridStep - 0.5);
        double top = std::floor(br.y() / gridStep + 1.0);

        // vertical lines
        for (int xi = int(left); xi <= int(right); ++xi) {
            QLineF line(xi * gridStep, bottom * gridStep, xi * gridStep, top * gridStep);

            painter->drawLine(line);
        }

        // horizontal lines
        for (int yi = int(bottom); yi <= int(top); ++yi) {
            QLineF line(left * gridStep, yi * gridStep, right * gridStep, yi * gridStep);
            painter->drawLine(line);
        }
    };

    auto const &flowViewStyle = QtNodes::StyleCollection::flowViewStyle();

    QPen pfine(flowViewStyle.FineGridColor, 1.0);

    painter->setPen(pfine);
    drawGrid(15);

    QPen p(flowViewStyle.CoarseGridColor, 1.0);

    painter->setPen(p);
    drawGrid(150);
}

GtIntelliGraphScene*
GtIntelliGraphView::nodeScene()
{
    return qobject_cast<GtIntelliGraphScene*>(scene());
}

QtNodes::DataFlowGraphModel*
GtIntelliGraphView::graphModel()
{
    if (auto scene = nodeScene())
    {
        return static_cast<QtNodes::DataFlowGraphModel*>(&scene->graphModel());
    }

    return nullptr;
}

void
GtIntelliGraphView::loadFromJson()
{
    if (!nodeScene()) return;

    QString filePath = GtFileDialog::getOpenFileName(nullptr, tr("Open Intelli Flow"));

    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) return;

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
    {
        gtError() << tr("Failed to open intelli flow from file! (%1)")
                         .arg(filePath);
        return;
    }

    auto scene = QJsonDocument::fromJson(file.readAll()).object();

    loadScene(scene);
}

void
GtIntelliGraphView::saveToJson()
{
    auto model = graphModel();
    if (!model) return;

    QString filePath = GtFileDialog::getSaveFileName(nullptr, tr("Save Intelli Flow"));

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        gtError() << tr("Failed to save IntelliFlow to file! (%1)")
                         .arg(filePath);
        return;
    }

    QJsonDocument doc(model->save());
    file.write(doc.toJson(QJsonDocument::Indented));
}

void
GtIntelliGraphView::loadScene(const QJsonObject& scene)
{
    assert(graphModel()); assert(nodeScene());

    gtDebug().verbose()
        << "Loading JSON scene:"
        << QJsonDocument(scene).toJson(QJsonDocument::Indented);

    try
    {
        nodeScene()->clearScene();
        graphModel()->load(scene);
    }
    catch (std::exception const& e)
    {
        gtError() << tr("Failed to load scene from object tree! Error:")
                  << gt::quoted(std::string{e.what()});
    }
}
