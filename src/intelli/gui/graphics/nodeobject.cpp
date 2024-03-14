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
    m_ui(&ui),
    m_evalStateObject(new NodeEvalStateGraphicsObject(*this, node, ui))
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

    Geometry geometry = m_ui->geometry(*m_node);
    repositionEvalStateVisualizer(geometry.evalStateVisualizerPosition());

//    embedCentralWidget();
}

Node*
NodeGraphicsObject::node()
{
    return m_node;
}

Node const*
NodeGraphicsObject::node() const
{
    return const_cast<NodeGraphicsObject*>(this)->node();
}

bool
NodeGraphicsObject::isHovered() const
{
    return m_hovered;
}

QRectF
NodeGraphicsObject::boundingRect() const
{
    return m_ui->geometry(*m_node).boundingRect();
}

void
NodeGraphicsObject::setNodeEvalState(NodeEvalState state)
{
    // forward node eval state
    m_evalStateObject->setNodeEvalState(state);
}

void
NodeGraphicsObject::repositionEvalStateVisualizer(QPointF pos)
{
    m_evalStateObject->setPos(pos);
}

void
NodeGraphicsObject::embedCentralWidget()
{
    // we may have to reembedd the widget
    if (m_proxyWidget)
    {
        m_proxyWidget->deleteLater();

        Geometry geometry = m_ui->geometry(*m_node);
        geometry.recomputeGeomtry();
    }

    if (auto w = m_node->embeddedWidget())
    {
        auto size = m_node->size();
        if (size.isValid()) w->resize(m_node->size());

        auto p = w->palette();
        p.setColor(QPalette::Window, m_ui->backgroundColor(*m_node));
        w->setPalette(p);

        Geometry geometry = m_ui->geometry(*m_node);

        m_proxyWidget = new QGraphicsProxyWidget(this);

        m_proxyWidget->setWidget(w);
        m_proxyWidget->setPreferredWidth(5);

        // compute geometry once widget was added
        geometry.recomputeGeomtry();

//        if (w->sizePolicy().verticalPolicy() & QSizePolicy::ExpandFlag)
//        {
//            unsigned int widgetHeight = geometry.size(_nodeId).height()
//                                        - geometry.captionRect(_nodeId).height();

//            // If the widget wants to use as much vertical space as possible, set
//            // it to have the geom's equivalentWidgetHeight.
//            m_proxyWidget->setMinimumHeight(widgetHeight);
//        }

        m_proxyWidget->setPos(geometry.widgetPosition());

        //update();

        m_proxyWidget->setOpacity(1.0);
        m_proxyWidget->setFlag(QGraphicsItem::ItemIgnoresParentOpacity);

        geometry.recomputeGeomtry();
    }
}

void
NodeGraphicsObject::paint(QPainter* painter,
                          QStyleOptionGraphicsItem const* option,
                          QWidget* widget)
{
    assert(painter);

    m_ui->painter(*this, *painter);
}

void
NodeGraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;

    QPointF coord = sceneTransform().inverted().map(event->scenePos());

    auto const& geometry = m_ui->geometry(*m_node);
    Geometry::PortHit hit = geometry.portHit(coord);

    if (hit)
    {
        auto const& connections = m_graph->findConnections(m_node->id(), hit.id);

        if (!connections.empty() && hit.type == PortType::In)
        {
            assert(connections.size() == 1);
            auto const &conId = connections.first();

            // TODO: disconnect connection and make draft
            m_graph->deleteConnection(conId);
            return;
        }

        // TODO: make draft connection
        return;
    }

    if (m_node->nodeFlags() & NodeFlag::Resizable)
    {
        auto pos = event->pos();
        bool hit = geometry.resizeHandleRect().contains(pos);
        if (hit)
        {
            m_state = Resizing;
            return;
        }
    }

    QGraphicsObject::mousePressEvent(event);

    if (isSelected())
    {
        emit gtApp->objectSelected(m_node);
    }
}

void
NodeGraphicsObject::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    // Deselect all other items after this one is selected.
    // Unless we press a CTRL button to add the item to the selected group before
    // starting moving.
    if (!isSelected())
    {
        if (!event->modifiers().testFlag(Qt::ControlModifier))
        {
            scene()->clearSelection();
        }
        setSelected(true);
    }

    if (m_state == Resizing)
    {
        auto diff = event->pos() - event->lastPos();

        if (auto w = m_node->embeddedWidget())
        {
            prepareGeometryChange();

            auto oldSize = w->size();

            oldSize += QSize(diff.x(), diff.y());

            w->resize(oldSize);

            // recompute node geometry
            Geometry geometry = m_ui->geometry(*m_node);
            geometry.recomputeGeomtry();
            update();

//            moveConnections();

            event->accept();
        }
        return;
    }

    m_state = Translating;

    QPointF diff = event->pos() - event->lastPos();
    setPos(pos() + diff);

    event->accept();
}

void
NodeGraphicsObject::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    switch (m_state)
    {
    case Translating:
        m_node->setPos(pos());
        m_state = Normal;
        break;
    case Resizing:
        if (m_proxyWidget)
        {
            m_node->setSize(m_proxyWidget->widget()->size());
        }
        m_state = Normal;
        break;
    case Normal:
        break;
    }

    QGraphicsObject::mouseReleaseEvent(event);

    // position connections precisely after fast node move
//    moveConnections();

    emit nodeClicked(m_node);
}

void
NodeGraphicsObject::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
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
NodeGraphicsObject::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    m_hovered = false;

    setZValue(0);

    update();

    event->accept();
}

void
NodeGraphicsObject::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    auto pos = event->pos();

    //NodeGeometry geometry(_nodeId, _graphModel, nodeScene());
    Geometry geometry = m_ui->geometry(*m_node);

    if (m_node->nodeFlags() & NodeFlag::Resizable &&
        geometry.resizeHandleRect().contains(pos))
    {
        setCursor(QCursor(Qt::SizeFDiagCursor));
    }
    else
    {
        setCursor(QCursor());
    }

    event->accept();
}

void
NodeGraphicsObject::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    gt::gui::handleObjectDoubleClick(*m_node);
}

void
NodeGraphicsObject::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    auto const& geometry = m_ui->geometry(*m_node);
    auto const& pos = event->pos();

    Geometry::PortHit hit = geometry.portHit(pos);

    if (!hit) return emit contextMenuRequested(m_node, pos);

    emit portContextMenuRequested(m_node, hit.id, pos);
}

