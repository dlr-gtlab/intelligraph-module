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
#include "intelli/gui/graphview.h"
#include "intelli/gui/graphscene.h"
#include "intelli/gui/style.h"
#include "intelli/gui/graphics/nodeobject.h"

#include <gt_filedialog.h>
#include <gt_grid.h>

#include <gt_logging.h>

#include <QCoreApplication>
#include <QMenu>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsWidget>
#include <QWheelEvent>
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
}

void
GraphView::setScene(GraphScene& scene)
{
    QGraphicsView::setScene(&scene);
    centerScene();
    setScale(1.0);
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

int
GraphView::minorGridSize() const
{
    return s_minor_grid_size;
}

int
GraphView::majorGridSize() const
{
    return s_major_grid_size;
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
    emit scaleChanged(transform().m11(), QPrivateSignal());
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
    emit scaleChanged(transform().m11(), QPrivateSignal());
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

    emit scaleChanged(scale, QPrivateSignal());
}

void
GraphView::printToPDF()
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
