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
#include <intelli/gui/nodeuidata.h>
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

        assert(parentItem());
        parentItem()->setSelected(true);

        return QGraphicsProxyWidget::mousePressEvent(event);
    }

};

struct NodeGraphicsObject::Impl
{
    /// Node eval state object
    NodeEvalStateGraphicsObject* evalStateObject = nullptr;
    /// Associated node
    QPointer<Node> node;
    /// Central widget
    QPointer<QGraphicsProxyWidget> proxyWidget;
    /// ui data
    std::unique_ptr<NodeUIData> uiData;
    /// Geometry
    std::unique_ptr<NodeGeometry> geometry;
    /// Painter
    std::unique_ptr<NodePainter> painter;
    /// Highlight data
    Highlights highlights;

    Impl(NodeGraphicsObject* obj, Node& node_) :
        node(&node_),
        highlights(*obj)
    {
    }

    /// Helper function that returns a scoped object which updates the geometry of
    /// the node accordingly
    GT_NO_DISCARD
    static inline auto
    prepareGeometryChange(NodeGraphicsObject* o)
    {
        assert(o);

        o->prepareGeometryChange();

        return gt::finally([o](){
            o->pimpl->geometry->recomputeGeometry();
            o->update();
            emit o->nodeGeometryChanged(o);
        });
    }

    /// Updates the palette of the widget
    static inline void
    updateWidgetPalette(NodeGraphicsObject* o)
    {
        assert(o);

        if (!o->pimpl->proxyWidget) return;
        QWidget* w = o->pimpl->proxyWidget->widget();
        if (!w) return;

        QPalette p = w->palette();
        p.setColor(QPalette::Window, o->pimpl->painter->backgroundColor());
        w->setPalette(p);
    }

}; // struct Impl;

NodeGraphicsObject::NodeGraphicsObject(GraphSceneData& data,
                                       Node& node,
                                       NodeUI& ui) :
    InteractableGraphicsObject(data, nullptr),
    pimpl(std::make_unique<Impl>(this, node))
{
    // impl must be created first
    pimpl->uiData = ui.uiData(node);
    pimpl->geometry = ui.geometry(*this);
    pimpl->painter = ui.painter(*this, *pimpl->geometry);
    pimpl->evalStateObject = new NodeEvalStateGraphicsObject(*this, *pimpl->painter, node);

    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setFlag(GraphicsItemFlag::ItemContainsChildrenInShape, true);

    setCacheMode(QGraphicsItem::DeviceCoordinateCache);

    setAcceptHoverEvents(true);

    setPos(pimpl->node->pos());

    embedCentralWidget();

    connect(this, &GraphicsObject::hoveredChanged, this, [this](){
        if (isHovered()) return setZValue(style::zValue(style::ZValue::NodeHovered));
        if (!isSelected()) return setZValue(style::zValue(style::ZValue::Node));
    });

    connect(this, &InteractableGraphicsObject::objectMoved, this, [this](){
        commitPosition();
    }, Qt::DirectConnection);

    connect(this, &InteractableGraphicsObject::objectCollapsed, this, [this](){
        if (auto w = centralWidget()) w->setVisible(!isCollapsed());
        Impl::prepareGeometryChange(this).finalize();
    }, Qt::DirectConnection);

    connect(this, &NodeGraphicsObject::nodeGeometryChanged,
            this, &NodeGraphicsObject::updateChildItems,
            Qt::DirectConnection);

    connect(&node, &Node::nodeChanged,
            this, &NodeGraphicsObject::onNodeChanged, Qt::DirectConnection);
    connect(&node, &Node::portChanged,
            this, &NodeGraphicsObject::onNodeChanged, Qt::DirectConnection);
    connect(&node, &Node::nodePositionChanged,
            this, &NodeGraphicsObject::onNodePositionChanged, Qt::DirectConnection);

    updateChildItems();
}

NodeGraphicsObject::~NodeGraphicsObject() = default;

Node&
NodeGraphicsObject::node()
{
    assert (pimpl->node);
    return *pimpl->node;
}

Node const&
NodeGraphicsObject::node() const
{
    return const_cast<NodeGraphicsObject*>(this)->node();
}

NodeId
NodeGraphicsObject::nodeId() const
{
    return pimpl->node->id();
}

ObjectUuid
NodeGraphicsObject::objectUuid() const
{
    return pimpl->node->uuid();
}

NodeUIData const&
NodeGraphicsObject::uiData() const
{
    return *pimpl->uiData;
}

bool
NodeGraphicsObject::hasResizeHandle() const
{
    return pimpl->node->nodeFlags() & IsResizableMask &&
           pimpl->proxyWidget && pimpl->proxyWidget->widget();
}

GraphicsObject::DeletableFlag
NodeGraphicsObject::deletableFlag() const
{
    if (node().objectFlags() & GtObject::ObjectFlag::UserDeletable)
    {
        return DefaultDeletable;
    }

    if (uiData().hasCustomDeleteFunction())
    {
        return NotBulkDeletable;
    }

    return NotDeletable;
}

GraphicsObject::DeleteOrdering
NodeGraphicsObject::deleteOrdering() const
{
    return DefaultDeleteOrdering;
}

bool
NodeGraphicsObject::deleteObject()
{
    if (uiData().hasCustomDeleteFunction())
    {
        return uiData().customDeleteFunction()(&node());
    }
    delete pimpl->node;
    return true;
}

QRectF
NodeGraphicsObject::boundingRect() const
{
    return pimpl->geometry->boundingRect();
}

QRectF
NodeGraphicsObject::widgetSceneBoundingRect() const
{
    if (!pimpl->proxyWidget || !pimpl->proxyWidget->widget()) return {};

    return pimpl->proxyWidget->sceneBoundingRect();
}

QPainterPath
NodeGraphicsObject::shape() const
{
    auto path = pimpl->geometry->shape();
    return path;
}

QGraphicsWidget*
NodeGraphicsObject::centralWidget()
{
    return pimpl->proxyWidget.data();
}

QGraphicsWidget const*
NodeGraphicsObject::centralWidget() const
{
    return const_cast<NodeGraphicsObject*>(this)->centralWidget();
}

NodeGraphicsObject::Highlights&
NodeGraphicsObject::highlights()
{
    return pimpl->highlights;
}

NodeGraphicsObject::Highlights const&
NodeGraphicsObject::highlights() const
{
    return pimpl->highlights;
}

NodeGeometry const&
NodeGraphicsObject::geometry() const
{
    assert(pimpl->geometry);
    return *pimpl->geometry;
}

void
NodeGraphicsObject::commitPosition()
{
    pimpl->node->setPos(pos());
}

void
NodeGraphicsObject::embedCentralWidget()
{
    auto const makeWidget = [this]() -> std::unique_ptr<QWidget> {
        auto factory = pimpl->node->pimpl->widgetFactory;
        if (!factory) return nullptr;

        auto widget = factory(*pimpl->node);
        if (!widget) return nullptr;

        auto size = pimpl->node->size();
        if (pimpl->node->nodeFlags() & IsResizableMask && size.isValid())
        {
            // resize only the adequate directions
            if (pimpl->node->nodeFlags() & NodeFlag::ResizableHOnly)
            {
                size.setHeight(widget->minimumSizeHint().height());
            }
            widget->resize(size);
        }
        return widget;
    };

    auto change = Impl::prepareGeometryChange(this);
    pimpl->geometry->recomputeGeometry();

    // we may have to reembedd the widget
    if (pimpl->proxyWidget)
    {
        pimpl->proxyWidget->deleteLater();
        pimpl->proxyWidget = nullptr;
    }

    if (auto w = makeWidget())
    {
        setFlag(GraphicsItemFlag::ItemContainsChildrenInShape, false);

        pimpl->geometry->setWidget(w.get());

        pimpl->proxyWidget = new NodeProxyWidget(this);
        pimpl->proxyWidget->setWidget(w.release());
        pimpl->proxyWidget->setZValue(style::zValue(style::ZValue::NodeWidget));

        Impl::updateWidgetPalette(this);

        // update node's size if widget changes size
        connect(pimpl->proxyWidget, &QGraphicsWidget::geometryChanged, this, [this](){
            if (state() == State::Resizing) return;

            if (pimpl->proxyWidget->widget() &&
                pimpl->node->nodeFlags() & IsResizableMask)
            {
                pimpl->node->setSize(pimpl->proxyWidget->widget()->size());
            }
            Impl::prepareGeometryChange(this).finalize();
        });
    }
}

void
NodeGraphicsObject::setupContextMenu(QMenu& menu)
{
    gt::gui::makeObjectContextMenu(menu, node());
}

void
NodeGraphicsObject::paint(QPainter* painter,
                          QStyleOptionGraphicsItem const* option,
                          QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    assert(painter);

    pimpl->painter->paint(*painter);
}

QVariant
NodeGraphicsObject::itemChange(GraphicsItemChange change, QVariant const& value)
{
    switch (change)
    {
    case GraphicsItemChange::ItemSelectedChange:
    {
        bool isSelected = value.toBool();
        setZValue(style::zValue(!isSelected ? style::ZValue::Node :
                                              style::ZValue::NodeHovered));
        break;
    }
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

    // check for port hit
    NodeGeometry::PortHit hit = pimpl->geometry->portHit(event->pos());
    if (hit)
    {
        event->accept();

        if (!pimpl->node->port(hit.port)) return;

        return emit makeDraftConnection(this, hit.type, hit.port);
    }

    // object will be selected
    if (!isSelected()) emit gtApp->objectSelected(pimpl->node);

    return InteractableGraphicsObject::mousePressEvent(event);
}

void
NodeGraphicsObject::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    bool isResizing = state() == State::Resizing;

    InteractableGraphicsObject::mouseReleaseEvent(event);

    if (isResizing)
    {
        assert(pimpl->proxyWidget);

        QWidget* w = pimpl->proxyWidget->widget();
        pimpl->node->setSize(w->size());
    }
}

void
NodeGraphicsObject::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    InteractableGraphicsObject::hoverEnterEvent(event);

    setToolTip(pimpl->node->tooltip());
}

void
NodeGraphicsObject::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    InteractableGraphicsObject::hoverMoveEvent(event);

    // set tooltip for ports
    NodeGeometry::PortHit hit = pimpl->geometry->portHit(event->pos());
    if (hit)
    {
        auto* port = pimpl->node->port(hit.port);
        assert(port);
        QString const& typeName = NodeDataFactory::instance().typeName(port->typeId);
        QString const& toolTip = port->toolTip.isEmpty() ?
                    typeName :
                    QStringLiteral("%1 (%2)")
                            .arg(port->toolTip, typeName);

        return setToolTip(toolTip);
    }

    setToolTip(pimpl->node->tooltip());
}

void
NodeGraphicsObject::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    InteractableGraphicsObject::hoverLeaveEvent(event);

    setToolTip({});
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

    // if (event->modifiers() & Qt::ControlModifier)
    // {
    //     setSelected(true);
    //     update();
    // }

    event->accept();
    NodeGeometry::PortHit hit = pimpl->geometry->portHit(pos);

    if (!hit)
    {
        emit contextMenuRequested(this);
    }
    else
    {
        emit portContextMenuRequested(this, hit.port);
    }
}

bool
NodeGraphicsObject::canResize(QPointF localCoord)
{
    return hasResizeHandle() && geometry().resizeHandleRect().contains(localCoord);
}

void
NodeGraphicsObject::resize(QSize diff)
{
    assert(pimpl->proxyWidget);
    QWidget* w = pimpl->proxyWidget->widget();
    assert(w);

    auto change = Impl::prepareGeometryChange(this);
    Q_UNUSED(change);

    QSize oldSize = w->size();
    oldSize.rwidth()  += diff.width();
    oldSize.rheight() += (pimpl->node->nodeFlags() & ResizableHOnly) ? 0 : diff.height();

    w->resize(oldSize);
}

void
NodeGraphicsObject::onNodeChanged()
{
    auto change = Impl::prepareGeometryChange(this);
    Q_UNUSED(change);
    
    pimpl->geometry->recomputeGeometry();
    updateChildItems();
}

void
NodeGraphicsObject::onNodePositionChanged()
{
    setPos(pimpl->node->pos());
    emit nodePositionChanged(this);
}

void
NodeGraphicsObject::updateChildItems()
{
    pimpl->evalStateObject->setPos(pimpl->geometry->evalStateRect().topLeft());
    if (pimpl->proxyWidget)
    {
        pimpl->proxyWidget->setPos(pimpl->geometry->widgetPosition());
    }
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

    auto& node = m_object->node();

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
