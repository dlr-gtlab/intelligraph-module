/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause AND LicenseRef-BSD-3-Clause-Dimitri
 *  SPDX-FileCopyrightText: 2022 Dimitri Pinaev
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */


#include <intelli/gui/graphics/nodeobject.h>

#include <intelli/node.h>
#include <intelli/gui/nodeui.h>
#include <intelli/gui/nodegeometry.h>
#include <intelli/gui/nodepainter.h>
#include <intelli/gui/graphscenedata.h>
#include <intelli/gui/graphics/nodeevalstateobject.h>
#include <intelli/gui/style.h>
#include <intelli/private/node_impl.h>
#include <intelli/nodedatafactory.h>

#include <gt_application.h>
#include <gt_guiutilities.h>

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsScene>

using namespace intelli;

// proxy widget to select node when clicking widget
class NodeGraphicsObject::NodeProxyWidget : public QGraphicsProxyWidget
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

        assert(qgraphicsitem_cast<NodeGraphicsObject*>(parentItem()));

        auto item = static_cast<NodeGraphicsObject*>(parentItem());
        item->selectNode();

        return QGraphicsProxyWidget::mousePressEvent(event);
    }

};

struct NodeGraphicsObject::Impl
{

/// Helper function that returns a scoped object which updates the geometry of
/// the node accordingly
GT_NO_DISCARD
static inline auto
prepareGeometryChange(NodeGraphicsObject* o)
{
    assert(o);

    o->prepareGeometryChange();

    return gt::finally([o](){
        o->m_geometry->recomputeGeomtry();
        o->update();
        emit o->nodeGeometryChanged(o);
    });
}

/// Updates the palette of the widget
static inline void
updateWidgetPalette(NodeGraphicsObject* o)
{
    assert(o);

    if (!o->m_proxyWidget) return;
    QWidget* w = o->m_proxyWidget->widget();
    if (!w) return;

    QPalette p = w->palette();
    p.setColor(QPalette::Window, o->m_painter->backgroundColor());
    w->setPalette(p);
}

}; // struct Impl;

NodeGraphicsObject::NodeGraphicsObject(GraphSceneData& data,
                                       Node& node,
                                       NodeUI& ui) :
    QGraphicsObject(nullptr),
    m_sceneData(&data),
    m_node(&node),
    m_geometry(ui.geometry(node)),
    m_painter(ui.painter(*this, *m_geometry)),
    m_evalStateObject(new NodeEvalStateGraphicsObject(*this, *m_painter, node)),
    m_highlights(*this)
{
    setFlag(QGraphicsItem::ItemContainsChildrenInShape, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);

    setCacheMode(QGraphicsItem::DeviceCoordinateCache);

    setAcceptHoverEvents(true);

    setZValue(style::zValue(style::ZValue::Node));
    setPos(m_node->pos());

    embedCentralWidget();

    connect(this, &NodeGraphicsObject::nodeGeometryChanged,
            this, &NodeGraphicsObject::updateChildItems,
            Qt::DirectConnection);

    connect(&node, &Node::nodeChanged,
            this, &NodeGraphicsObject::onNodeChanged, Qt::DirectConnection);
    connect(&node, &Node::portChanged,
            this, &NodeGraphicsObject::onNodeChanged, Qt::DirectConnection);

    updateChildItems();
}

NodeGraphicsObject::~NodeGraphicsObject() = default;

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

GraphSceneData const&
NodeGraphicsObject::sceneData() const
{
    return *m_sceneData;
}

bool
NodeGraphicsObject::isHovered() const
{
    return m_hovered;
}

bool
NodeGraphicsObject::hasResizeHandle() const
{
    return m_node->nodeFlags() & IsResizableMask &&
           m_proxyWidget && m_proxyWidget->widget();
}

QRectF
NodeGraphicsObject::boundingRect() const
{
    return m_geometry->boundingRect();
}

QPainterPath
NodeGraphicsObject::shape() const
{
    auto path = m_geometry->shape();
    return path;
}

QGraphicsWidget*
NodeGraphicsObject::centralWidget()
{
    return m_proxyWidget.data();
}

QGraphicsWidget const*
NodeGraphicsObject::centralWidget() const
{
    return const_cast<NodeGraphicsObject*>(this)->centralWidget();
}

NodeGraphicsObject::Highlights&
NodeGraphicsObject::highlights()
{
    return m_highlights;
}

NodeGraphicsObject::Highlights const&
NodeGraphicsObject::highlights() const
{
    return m_highlights;
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
    auto const makeWidget = [this]() -> std::unique_ptr<QWidget> {
        auto factory = m_node->pimpl->widgetFactory;
        if (!factory) return nullptr;

        auto widget = factory(*m_node);
        if (!widget) return nullptr;

        auto size = m_node->size();
        if (m_node->nodeFlags() & IsResizableMask && size.isValid())
        {
            // resize only the adequate directions
            if (m_node->nodeFlags() & NodeFlag::ResizableHOnly)
            {
                size.setHeight(widget->minimumSizeHint().height());
            }
            widget->resize(size);
        }
        return widget;
    };

    auto change = Impl::prepareGeometryChange(this);
    m_geometry->recomputeGeomtry();

    // we may have to reembedd the widget
    if (m_proxyWidget)
    {
        m_proxyWidget->deleteLater();
        m_proxyWidget = nullptr;
    }

    if (auto w = makeWidget())
    {
        m_geometry->setWidget(w.get());

        m_proxyWidget = new NodeProxyWidget(this);
        m_proxyWidget->setWidget(w.release());
        m_proxyWidget->setZValue(style::zValue(style::ZValue::NodeWidget));

        Impl::updateWidgetPalette(this);

        // update node's size if widget changes size
        connect(m_proxyWidget, &QGraphicsWidget::geometryChanged, this, [this](){
            if (m_state == Resizing) return;

            if (m_proxyWidget->widget() &&
                m_node->nodeFlags() & IsResizableMask)
            {
                m_node->setSize(m_proxyWidget->widget()->size());
            }
            Impl::prepareGeometryChange(this).finalize();
        });
    }
}

void
NodeGraphicsObject::commitPosition()
{
    m_node->setPos(pos());
}

void
NodeGraphicsObject::paint(QPainter* painter,
                          QStyleOptionGraphicsItem const* option,
                          QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    assert(painter);

    m_painter->paint(*painter);
}

QVariant
NodeGraphicsObject::itemChange(GraphicsItemChange change, const QVariant& value)
{
    switch (change)
    {
    case GraphicsItemChange::ItemSelectedChange:
        setZValue(style::zValue(!value.toBool() ?
                                    style::ZValue::Node :
                                    style::ZValue::NodeHovered));
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
    Q_UNUSED(accept);

    // bring this node forward
    setZValue(style::zValue(style::ZValue::NodeHovered));

    QPointF coord = sceneTransform().inverted().map(event->scenePos());

    // check for port hit
    NodeGeometry::PortHit hit = m_geometry->portHit(coord);
    if (hit)
    {
        if (!m_node->port(hit.port)) return;

        return emit makeDraftConnection(this, hit.type, hit.port);
    }

    // check for resize handle hit
    if (hasResizeHandle())
    {
        bool resize = m_geometry->resizeHandleRect().contains(event->pos());
        if (resize)
        {
            m_state = Resizing;
            return;
        }
    }

    bool select = !(event->modifiers() & Qt::ControlModifier);

    // clear selection
    if (!isSelected())
    {
        select = true;
        if (!(event->modifiers() & Qt::ControlModifier))
        {
            auto* scene = this->scene();
            assert(scene);
            scene->clearSelection();
        }
    }

    if (select) selectNode();

    m_state = Translating;
    m_translationDiff = pos();
}

void
NodeGraphicsObject::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QPointF diff = event->pos() - event->lastPos();
    m_translationDiff += diff;

    switch (m_state)
    {
    case Resizing:
        if (m_proxyWidget)
        if (auto w = m_proxyWidget->widget())
        {
            auto change = Impl::prepareGeometryChange(this);
            Q_UNUSED(change);

            QSize oldSize = w->size();
            oldSize += QSize(diff.x(), (m_node->nodeFlags() & ResizableHOnly) ? 0 : diff.y());

            w->resize(oldSize);
        }
        return event->accept();

    case Translating:
        if ((m_sceneData->snapToGrid || event->modifiers() & Qt::ControlModifier)
            && m_sceneData->gridSize > 0)
        {
            QPoint newPos = quantize(m_translationDiff, m_sceneData->gridSize);

            // position not changed
            if (pos() == newPos) return event->accept();

            diff = newPos - pos();
        }

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
        setZValue(style::zValue(style::ZValue::Node));
    }

    event->accept();

}

void
NodeGraphicsObject::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    setToolTip(m_node->tooltip());

    setZValue(style::zValue(style::ZValue::NodeHovered));

    m_hovered = true;
    update();

    event->accept();
}

void
NodeGraphicsObject::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    QPointF pos = event->pos();

    auto accept = gt::finally(event, &QEvent::accept);
    Q_UNUSED(accept);

    // check for resize handle hit and change cursor
    if (hasResizeHandle() && m_geometry->resizeHandleRect().contains(pos))
    {
        setCursor(QCursor(Qt::SizeFDiagCursor));
        return;
    }

    setCursor(QCursor());

    // set tooltip for ports
    NodeGeometry::PortHit hit = m_geometry->portHit(pos);
    if (hit)
    {
        auto* port = m_node->port(hit.port);
        assert(port);
        QString const& typeName = NodeDataFactory::instance().typeName(port->typeId);
        QString const& toolTip = port->toolTip.isEmpty() ?
                    typeName :
                    QStringLiteral("%1 (%2)")
                            .arg(port->toolTip, typeName);

        return setToolTip(toolTip);
    }

    setToolTip(m_node->tooltip());
}

void
NodeGraphicsObject::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    setToolTip({});

    if (!isSelected())
    {
        setZValue(style::zValue(style::ZValue::Node));
    }

    m_hovered = false;
    update();

    event->accept();
}


void
NodeGraphicsObject::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    emit nodeDoubleClicked(this);
    event->accept();
}

void
NodeGraphicsObject::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    auto const& pos = event->pos();

    if (event->modifiers() & Qt::ControlModifier)
    {
        setSelected(true);
        update();
    }

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
NodeGraphicsObject::selectNode()
{
    setSelected(true);
    emit gtApp->objectSelected(m_node);
}

void
NodeGraphicsObject::onNodeChanged()
{
    auto change = Impl::prepareGeometryChange(this);
    Q_UNUSED(change);

    m_geometry->recomputeGeomtry();
    updateChildItems();
}

void
NodeGraphicsObject::updateChildItems()
{
    m_evalStateObject->setPos(m_geometry->evalStateRect().topLeft());
    if (m_proxyWidget) m_proxyWidget->setPos(m_geometry->widgetPosition());
}

NodeGraphicsObject::Highlights::Highlights(NodeGraphicsObject& object) :
    m_object(&object)
{
    assert(m_object);
}

bool
NodeGraphicsObject::Highlights::isActive() const
{
    return m_isActive;
}

bool
NodeGraphicsObject::Highlights::isNodeCompatible() const
{
    return m_isNodeCompatible && !m_compatiblePorts.empty();
}

bool
NodeGraphicsObject::Highlights::isPortCompatible(PortId port) const
{
    return m_compatiblePorts.contains(port);
}

void
NodeGraphicsObject::Highlights::setAsIncompatible()
{
    m_isActive = true;
    m_isNodeCompatible = false;

    m_compatiblePorts.clear();

    Impl::updateWidgetPalette(m_object);

    m_object->update();
}

void
NodeGraphicsObject::Highlights::setCompatiblePorts(TypeId const& typeId,
                                                   PortType type)
{
    m_isActive = true;
    m_isNodeCompatible = true;

    auto& node  = m_object->node();

    auto& factory = NodeDataFactory::instance();
    for (auto& port : node.ports(type))
    {
        // check whether port is already connected
        if (type == PortType::In && port.isConnected()) continue;

        if (!factory.canConvert(port.typeId, typeId, type)) continue;

        m_compatiblePorts.append(port.id());
    }

    if (isNodeCompatible()) Impl::updateWidgetPalette(m_object);
    m_object->update();
}

void
NodeGraphicsObject::Highlights::setPortAsCompatible(PortId port)
{
    m_compatiblePorts.append(port);
}

void
NodeGraphicsObject::Highlights::clear()
{
    m_isActive = false;
    m_compatiblePorts.clear();

    Impl::updateWidgetPalette(m_object);

    m_object->update();
}
