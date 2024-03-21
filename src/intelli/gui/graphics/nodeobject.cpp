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
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsScene>

using namespace intelli;

class intelli::NodeProxyWidget : public QGraphicsProxyWidget
{
public:

    using QGraphicsProxyWidget::QGraphicsProxyWidget;

protected:

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (!isSelected() && !event->modifiers().testFlag(Qt::ControlModifier))
        {
            auto* scene = this->scene();
            assert(scene);
            scene->clearSelection();
        }

        parentItem()->setSelected(true);

        return QGraphicsProxyWidget::mousePressEvent(event);
    }

};

struct NodeGraphicsObject::Impl
{

GT_NO_DISCARD
static inline auto prepareGeometryChange(NodeGraphicsObject* o)
{
    o->prepareGeometryChange();
    o->m_geometry->recomputeGeomtry();

    return gt::finally([o](){
        o->update();
        emit o->nodeGeometryChanged(o);
    });
}

}; // struct Impl;

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
    setFlag(QGraphicsItem::ItemIsMovable, true);
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

NodeId
NodeGraphicsObject::nodeId() const
{
    return m_node->id();
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

QPainterPath
NodeGraphicsObject::shape() const
{
    return m_geometry->shape();
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

NodeGeometry const&
NodeGraphicsObject::geometry() const
{
    assert(m_geometry);
    return *m_geometry;
}

void
NodeGraphicsObject::embedCentralWidget()
{
    auto change = Impl::prepareGeometryChange(this);

    // we may have to reembedd the widget
    if (m_proxyWidget)
    {
        m_proxyWidget->deleteLater();
        m_proxyWidget = nullptr;
    }

    if (auto w = m_node->makeWidget())
    {
        auto size = m_node->size();
        if (size.isValid()) w->resize(m_node->size());

        auto p = w->palette();
        p.setColor(QPalette::Window, m_painter->backgroundColor());
        w->setPalette(p);

        m_proxyWidget = new NodeProxyWidget(this);

        m_proxyWidget->setWidget(w);
        m_proxyWidget->setPreferredWidth(5);
        m_proxyWidget->setOpacity(1.0);
        m_proxyWidget->setFlag(QGraphicsItem::ItemIgnoresParentOpacity);

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
        // send node back
        if (!value.toBool()) setZValue(0);
        break;
    default:
        break;
    }

    return value;
}

void
NodeGraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
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
        auto const& connections = m_graph->findConnections(m_node->id(), hit.port);

        if (!connections.empty() && hit.type == PortType::In)
        {
            assert(connections.size() == 1);
            auto const& conId = connections.first();

            emit makeDraftConnection(this, conId);
            return;
        }
        
        emit makeDraftConnection(this, hit.type, hit.port);
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
        auto* scene = this->scene();
        assert(scene);
        scene->clearSelection();
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
                auto change = Impl::prepareGeometryChange(this);

            auto oldSize = w->size();

            oldSize += QSize(diff.x(), diff.y());

            w->resize(oldSize);
        }
        return event->accept();

    case Translating:
        moveBy(diff.x(), diff.y());
        emit nodeShifted(this, diff);
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
        emit nodeMoved(this);
        break;
    }

    m_state = Normal;

    if (!isSelected())
    {
        // send node back
        setZValue(0);
    }

    event->accept();
}

void
NodeGraphicsObject::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    // bring this node forward
    setZValue(1.0);

    m_hovered = true;
    update();

    event->accept();
}

void
NodeGraphicsObject::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    event->accept();

    if (m_node->nodeFlags() & NodeFlag::Resizable &&
        m_node->embeddedWidget() &&
        m_geometry->resizeHandleRect().contains(event->pos()))
    {
        return setCursor(QCursor(Qt::SizeFDiagCursor));
    }

    setCursor(QCursor());
}

void
NodeGraphicsObject::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
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
        emit contextMenuRequested(this, pos);
    }
    else
    {
        emit portContextMenuRequested(this, hit.port, pos);
    }

    event->accept();
}

void
NodeGraphicsObject::onNodeChanged()
{
    auto change = Impl::prepareGeometryChange(this);

    m_proxyWidget->setPos(m_geometry->widgetPosition());
}

