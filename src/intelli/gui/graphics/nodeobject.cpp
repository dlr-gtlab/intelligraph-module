/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 12.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/graphics/nodeevalstateobject.h>
#include <intelli/gui/style.h>
#include <intelli/graph.h>

#include <gt_application.h>
#include <gt_guiutilities.h>
#include <gt_colors.h>

#include <QPainter>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>

using namespace intelli;

NodeGraphicsObject::NodeGraphicsObject(Graph& graph, Node& node, NodeUI& ui) :
    QGraphicsObject(nullptr),
    m_graph(&graph),
    m_node(&node),
    m_proxyWidget(nullptr),
    m_geometry(ui.geometry(node)),
    m_painter(ui.painter(*this, *m_geometry)),
    m_evalStateObject(new NodeEvalStateGraphicsObject(*this, node, *m_painter))
{
    setFlag(QGraphicsItem::ItemDoesntPropagateOpacityToChildren, true);
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);

    setCacheMode(QGraphicsItem::DeviceCoordinateCache);

    setAcceptHoverEvents(true);

    setZValue(0);
    setOpacity(style::nodeOpacity());
    setPos(m_node->pos());

    m_evalStateObject->setPos(m_geometry->evalStateVisualizerPosition());

    embedCentralWidget();
}

Node&
NodeGraphicsObject::node()
{
    assert (m_node);
    return *m_node;
}

Node const&
NodeGraphicsObject::node() const
{
    return const_cast<NodeGraphicsObject*>(this)->node();
}

Graph&
NodeGraphicsObject::graph()
{
    assert (m_graph);
    return *m_graph;
}

Graph const&
NodeGraphicsObject::graph() const
{
    return const_cast<NodeGraphicsObject*>(this)->graph();
}

bool
NodeGraphicsObject::isHovered() const
{
    return m_hovered;
}

QRectF
NodeGraphicsObject::boundingRect() const
{
    return m_geometry->boundingRect();
}

QGraphicsWidget*
NodeGraphicsObject::centralWidget()
{
    return m_proxyWidget;
}

QGraphicsWidget const*
NodeGraphicsObject::centralWidget() const
{
    return const_cast<NodeGraphicsObject*>(this)->centralWidget();
}

void
NodeGraphicsObject::embedCentralWidget()
{
    // we may have to reembedd the widget
    if (m_proxyWidget)
    {
        m_proxyWidget->deleteLater();
        m_proxyWidget = nullptr;

        m_geometry->recomputeGeomtry();
    }

    if (auto w = m_node->makeWidget())
    {
        auto size = m_node->size();
        if (size.isValid()) w->resize(m_node->size());

        auto p = w->palette();
        p.setColor(QPalette::Window, m_painter->backgroundColor());
        w->setPalette(p);

        m_proxyWidget = new QGraphicsProxyWidget(this);

        m_proxyWidget->setWidget(w);
        m_proxyWidget->setPreferredWidth(5);
        m_proxyWidget->setOpacity(1.0);
        m_proxyWidget->setFlag(QGraphicsItem::ItemIgnoresParentOpacity);

        m_geometry->recomputeGeomtry();

        m_proxyWidget->setPos(m_geometry->widgetPosition());
    }
}

void
NodeGraphicsObject::commitPosition()
{
    m_node->setPos(pos());
}

void
NodeGraphicsObject::setNodeEvalState(NodeEvalState state)
{
    // forward node eval state
    m_evalStateObject->setNodeEvalState(state);
}

void
NodeGraphicsObject::paint(QPainter* painter,
                          QStyleOptionGraphicsItem const* option,
                          QWidget* widget)
{
    assert(painter);

    m_painter->paint(*painter);
}

QVariant
NodeGraphicsObject::itemChange(GraphicsItemChange change, const QVariant& value)
{
    switch (change)
    {
    case GraphicsItemChange::ItemSelectedChange:
        if (!value.toBool())
        {
            // send node back
            setZValue(0);
        }
        break;
    default:
        break;
    }

    return value;
}

void
NodeGraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() != Qt::LeftButton ||
        !m_geometry->innerRect().contains(event->pos()))
    {
        return event->ignore();
    }

    auto accept = gt::finally(event, &QEvent::accept);

    // bring this node forward
    setZValue(1.0);

    QPointF coord = sceneTransform().inverted().map(event->scenePos());

    NodeGeometry::PortHit hit = m_geometry->portHit(coord);

    if (hit)
    {
        auto const& connections = m_graph->findConnections(m_node->id(), hit.id);

        if (!connections.empty() && hit.type == PortType::In)
        {
            assert(connections.size() == 1);
            auto const &conId = connections.first();

            // TODO: disconnect connection and make draft
            m_graph->deleteConnection(conId);
        }

        // TODO: make draft connection
        return;
    }

    if (m_node->nodeFlags() & NodeFlag::Resizable && m_node->embeddedWidget())
    {
        auto pos = event->pos();
        bool hit = m_geometry->resizeHandleRect().contains(pos);
        if (hit)
        {
            m_state = Resizing;
            return;
        }
    }

    if (!isSelected() && !event->modifiers().testFlag(Qt::ControlModifier))
    {
        scene()->clearSelection();
    }

    setSelected(true);
    emit gtApp->objectSelected(m_node);

    m_state = Translating;
}

void
NodeGraphicsObject::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    auto diff = event->pos() - event->lastPos();

    switch (m_state)
    {
    case Resizing:
        if (m_proxyWidget)
        if (auto w = m_proxyWidget->widget())
        {
            prepareGeometryChange();

            auto oldSize = w->size();

            oldSize += QSize(diff.x(), diff.y());

            w->resize(oldSize);

            // recompute node geometry
            m_geometry->recomputeGeomtry();
            update();

//            moveConnections();
        }
        return event->accept();

    case Translating:
        moveBy(diff.x(), diff.y());
        emit nodeShifted(diff);
        return event->accept();

    case Normal:
    default:
        return event->ignore();
    }
}

void
NodeGraphicsObject::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    switch (m_state)
    {
    case Normal:
    default:
        return event->ignore();

    case Resizing:
        if (m_proxyWidget)
        {
            m_node->setSize(m_proxyWidget->widget()->size());
        }
        m_state = Normal;
        break;

    case Translating:
        commitPosition();
        emit nodeMoved();
        break;
    }

    m_state = Normal;

    if (!isSelected())
    {
        // send node back
        setZValue(0);
    }

    event->accept();

//    if (!m_geometry->innerRect().contains(event->pos())) return;

//    QGraphicsObject::mouseReleaseEvent(event);

    // position connections precisely after fast node move
//    moveConnections();
}

void
NodeGraphicsObject::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    if (!m_geometry->innerRect().contains(event->pos())) return event->ignore();

//    // bring all the colliding nodes to background
//    QList<QGraphicsItem *> overlapItems = collidingItems();

//    for (QGraphicsItem *item : overlapItems)
//    {
//        if (item->zValue() > 0.0)
//        {
//            item->setZValue(0.0);
//        }
//    }

    // bring this node forward
    setZValue(1.0);

    m_hovered = true;
    update();

    event->accept();
}

void
NodeGraphicsObject::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    auto pos = event->pos();

    if (!m_geometry->innerRect().contains(pos))
    {
        setCursor(QCursor(Qt::OpenHandCursor));
        if (m_hovered)
        {
            hoverLeaveEvent(event);
        }
        return event->ignore();
    }

    auto accept = gt::finally(event, &QEvent::accept);

    if (!m_hovered)
    {
        m_hovered = true;
        update();
    }

    if (m_node->nodeFlags() & NodeFlag::Resizable &&
        m_node->embeddedWidget() &&
        m_geometry->resizeHandleRect().contains(pos))
    {
        return setCursor(QCursor(Qt::SizeFDiagCursor));
    }

    setCursor(QCursor());
}

void
NodeGraphicsObject::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    if (!m_hovered) return event->ignore();

    if (!isSelected())
    {
        // send node back
        setZValue(0);
    }

    m_hovered = false;
    update();

    event->accept();
}


void
NodeGraphicsObject::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if (!m_geometry->innerRect().contains(event->pos())) return event->ignore();

    gt::gui::handleObjectDoubleClick(*m_node);

    event->accept();
}

void
NodeGraphicsObject::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    auto const& pos = event->pos();

    NodeGeometry::PortHit hit = m_geometry->portHit(pos);

    if (!hit)
    {
        if (!m_geometry->innerRect().contains(pos)) return event->ignore();

        emit contextMenuRequested(m_node, pos);
    }
    else
    {
        emit portContextMenuRequested(m_node, hit.id, pos);
    }

    event->accept();
}

void
NodeGraphicsObject::onNodeChanged()
{
    prepareGeometryChange();
    m_geometry->recomputeGeomtry();
    m_proxyWidget->setPos(m_geometry->widgetPosition());
}

