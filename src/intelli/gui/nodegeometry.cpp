/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 26.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/gui/nodegeometry.h"

#include "intelli/nodedatafactory.h"
#include "intelli/node.h"
#include "intelli/gui/style.h"

using namespace intelli;

NodeGeometry::NodeGeometry(Node& node) :
    m_node(&node)
{

}

void
NodeGeometry::setWidget(QPointer<QWidget> widget)
{
    m_widget = widget;
}

bool
NodeGeometry::positionWidgetAtBottom() const
{
    return node().nodeFlags() & NodeFlag::MaximizeWidget;
}

int
NodeGeometry::hspacing() const
{
    return 10;
}

int
NodeGeometry::vspacing() const
{
    return 0.5 * hspacing();
}

int
NodeGeometry::captionHeightExtend() const
{
    QRectF caption = captionRect();
    return caption.height() + 2 * caption.topLeft().y();
}

int
NodeGeometry::portHorizontalExtent(PortType type) const
{
    int advance = 0;

    auto n = node().ports(type).size();
    for (PortIndex idx{0}; idx < n; ++idx)
    {
        advance = std::max(advance, (int)portCaptionRect(type, idx).width());
    }
    return advance;
}

int
NodeGeometry::portHeightExtent() const
{
    QRectF caption = captionRect();
    int height = caption.height() + caption.topLeft().y();

    for (PortType type : {PortType::In, PortType::Out})
    {
        auto n = node().ports(type).size();
        if (n == 0) continue;

        height = std::max(height, (int)(portCaptionRect(type, PortIndex::fromValue(n - 1)).bottomLeft().y() + vspacing()));
    }

    return height;
}

QPainterPath const&
NodeGeometry::shape() const
{
    if (m_shape.has_value()) return *m_shape;

    // set empty value to avoid cyclic calls
    m_shape = QPainterPath{};
    m_shape = computeShape();
    return *m_shape;
}

QPainterPath
NodeGeometry::computeShape() const
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

QRectF
NodeGeometry::innerRect() const
{
    if (m_innerRect.has_value()) return *m_innerRect;

    // set empty value to avoid cyclic calls
    m_innerRect = QRectF{};
    m_innerRect = computeInnerRect();
    return *m_innerRect;
}

QRectF
NodeGeometry::computeInnerRect() const
{
    QSize wSize{0, 0};
    if (auto w = widget())
    {
        wSize = w->size();
    }
    bool positionAtBottom = positionWidgetAtBottom();

    // height
    int height = vspacing() + portHeightExtent();

    // width
    int width = 0;
    for (PortType type : {PortType::In, PortType::Out})
    {
        width += portHorizontalExtent(type) + hspacing();
    }

    if (positionAtBottom)
    {
        height += wSize.height();
        width  += hspacing();
        width   = std::max(width, (int)wSize.width() + hspacing());
    }
    else
    {
        height = std::max(height, captionHeightExtend() + wSize.height() + vspacing());
        width += hspacing() + wSize.width();
    }

    width = std::max(width, (int)(style::nodeEvalStateSize() + hspacing() + captionRect().width()));

    return QRectF(QPoint{0, 0}, QSize{width, height});
}

QRectF
NodeGeometry::boundingRect() const
{
    if (m_boundingRect.has_value()) return *m_boundingRect;

    // set empty value to avoid cyclic calls
    m_boundingRect = QRectF{};
    m_boundingRect = computeBoundingRect();
    return *m_boundingRect;
}

QRectF
NodeGeometry::computeBoundingRect() const
{
    double xoffset = 0.6 * style::nodePortSize() * 2;
    double yoffset = 0.6 * style::nodePortSize() * 2;

    auto rect = innerRect();

    return QRectF(rect.topLeft() - QPointF{xoffset, yoffset},
                  QSizeF{rect.width()  + 2 * xoffset,
                         rect.height() + 2 * yoffset});
}

QRectF
NodeGeometry::captionRect() const
{
    QFont f;
    f.setBold(true);
    QFontMetrics metrics{f};

    constexpr int errorMargin = 2;

    // center caption
    int width = metrics.horizontalAdvance(node().caption());
    width += (width & 1) + errorMargin;

    QRectF innerRect = this->innerRect();
    QRectF caption{innerRect.topLeft(), QSize{width, metrics.height()}};

    double margin = innerRect.width() - caption.width();

    double xoffset = 0.5 * (innerRect.width() - caption.width());

    // make the caption as centered as possible
    xoffset += (style::nodeEvalStateSize() / margin) * 0.5 * style::nodeEvalStateSize();

    return caption.translated(xoffset, vspacing());
}

QRectF
NodeGeometry::evalStateRect() const
{
    return QRectF{
        innerRect().topLeft(),
        QSizeF{style::nodeEvalStateSize(), style::nodeEvalStateSize()}
    };
}

QPointF
NodeGeometry::widgetPosition() const
{
    auto* w = widget();
    if (!w) return {};

    if (positionWidgetAtBottom())
    {
        double xOffset = 0.5 * (innerRect().width() - w->width());
        double yOffset = portHeightExtent();

        return QPointF{xOffset, yOffset};
    }

    double xOffset = 1.5 * hspacing() + portHorizontalExtent(PortType::In);
    double yOffset = captionHeightExtend();

    if (node().ports(PortType::In).empty())
    {
        xOffset = innerRect().width() - w->width() - 1.5 * hspacing() - portHorizontalExtent(PortType::Out);
    }

    return QPointF{xOffset, yOffset};
}

QRectF
NodeGeometry::portRect(PortType type, PortIndex idx) const
{
    assert(type != PortType::NoType && idx != invalid<PortIndex>());

    QFontMetrics metrics{QFont()};

    double height = captionHeightExtend() + vspacing();

    for (PortIndex i{0}; i < idx; ++i)
    {
        height += 1.5 * metrics.height();
    }

    double width = type == PortType::Out ? innerRect().width() : 0.0;
    width -= style::nodePortSize();

    return {
        QPointF(width, height),
        QSizeF{style::nodePortSize() * 2, style::nodePortSize() * 2}
    };
}

QRectF
NodeGeometry::portCaptionRect(PortType type, PortIndex idx) const
{
    assert(type != PortType::NoType && idx != invalid<PortIndex>());

    QFontMetrics metrics{QFont()};

    auto& factory = NodeDataFactory::instance();

    auto& node = this->node();
    auto* port = node.port(node.portId(type, idx));
    assert(port);

    int height = metrics.height();
    int width = 0;

    if (port->captionVisible)
    {
        width += metrics.horizontalAdvance(port->caption.isEmpty() ? factory.typeName(port->typeId) : port->caption);
        width += (width & 1);
    }

    QPointF pos = portRect(type, idx).center();
    pos.setY(pos.y() - height * 0.5);
    pos.setX(type == PortType::In ?
                 pos.x() + hspacing() :
                 pos.x() - hspacing() - width);

    return QRectF{pos, QSize{width, height}};
}

NodeGeometry::PortHit
NodeGeometry::portHit(QPointF coord) const
{
    constexpr QPointF offset{0.5, 0.5};
    return portHit(QRectF{coord - offset, coord + offset});
}

NodeGeometry::PortHit
NodeGeometry::portHit(QRectF rect) const
{
    auto inner = innerRect();
    auto coord = rect.center();

    // estimate whether its a input or output port
    PortType type = (coord.x() < (inner.x() + 0.5 * inner.width())) ? PortType::In : PortType::Out;

    auto& node = this->node();

    // check each port
    for (auto& port : node.ports(type))
    {
        auto pRect = this->portRect(type, node.portIndex(type, port.id()));
        if (pRect.intersects(rect))
        {
            return PortHit{type, port.id()};
        }
    }

    return {};
}

QRectF
NodeGeometry::resizeHandleRect() const
{
    constexpr QSize size{8, 8};

    QRectF rect = innerRect();
    return QRectF(rect.bottomRight() - QPoint{size.width(), size.height()}, size);
}

void
NodeGeometry::recomputeGeomtry()
{
    m_innerRect.reset();
    m_shape.reset();
    m_boundingRect.reset();
}

Node&
NodeGeometry::node() const
{
    assert(m_node);
    return *m_node;
}

QWidget const*
NodeGeometry::widget() const
{
    return m_widget;
}

