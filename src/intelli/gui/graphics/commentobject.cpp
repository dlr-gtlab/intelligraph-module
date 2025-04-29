/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/graphics/commentobject.h>
#include <intelli/gui/style.h>

#include <gt_application.h>
#include <gt_colors.h>

#include <QPainter>
#include <QTextEdit>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>

using namespace intelli;

class CommentGraphicsObject::ProxyWidget : public QGraphicsProxyWidget
{
public:

    using QGraphicsProxyWidget::QGraphicsProxyWidget;

    bool frwd = false;

protected:

    void keyPressEvent(QKeyEvent* event) override
    {
        if (frwd) return QGraphicsProxyWidget::keyPressEvent(event);

        static_cast<CommentGraphicsObject*>(parentObject())->keyPressEvent(event);
    }

    void keyReleaseEvent(QKeyEvent* event) override
    {
        if (frwd) return QGraphicsProxyWidget::keyReleaseEvent(event);

        static_cast<CommentGraphicsObject*>(parentObject())->keyReleaseEvent(event);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (frwd) return QGraphicsProxyWidget::mousePressEvent(event);

        static_cast<CommentGraphicsObject*>(parentObject())->mousePressEvent(event);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (frwd) return QGraphicsProxyWidget::mouseMoveEvent(event);

        static_cast<CommentGraphicsObject*>(parentObject())->mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (frwd) return QGraphicsProxyWidget::mouseReleaseEvent(event);

        static_cast<CommentGraphicsObject*>(parentObject())->mouseReleaseEvent(event);
    }

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override
    {
        if (frwd) return QGraphicsProxyWidget::mouseDoubleClickEvent(event);

        static_cast<CommentGraphicsObject*>(parentObject())->mouseDoubleClickEvent(event);
    }

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override
    {
        if (frwd) return QGraphicsProxyWidget::hoverEnterEvent(event);

        static_cast<CommentGraphicsObject*>(parentObject())->hoverEnterEvent(event);
    }

    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override
    {
        if (frwd) return QGraphicsProxyWidget::hoverMoveEvent(event);

        static_cast<CommentGraphicsObject*>(parentObject())->hoverMoveEvent(event);
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override
    {
        if (frwd) return QGraphicsProxyWidget::hoverLeaveEvent(event);

        static_cast<CommentGraphicsObject*>(parentObject())->hoverLeaveEvent(event);
    }
};


CommentGraphicsObject::CommentGraphicsObject()
{
    setFlag(GraphicsItemFlag::ItemIsSelectable, true);

    setAcceptHoverEvents(true);

    editor = new QTextEdit;
    editor->setMarkdown(tr("### Hello World\n\nthis is some text\n\n`this is code`\n"));
    editor->setFrameShape(QFrame::NoFrame);
    editor->setContextMenuPolicy(Qt::NoContextMenu);

    auto* widget = new ProxyWidget(this);
    widget->setWidget(editor);

    proxyWidget = widget;
}

QRectF
CommentGraphicsObject::boundingRect() const
{
    QRectF rect = proxyWidget->boundingRect();
    return rect;
}

QRectF
CommentGraphicsObject::resizeHandleRect() const
{
    constexpr QSize size{8, 8};

    QRectF body = boundingRect();
    return QRectF(body.bottomRight() - QPoint{size.width(), size.height()}, size);
}

QVariant
CommentGraphicsObject::itemChange(GraphicsItemChange change,
                                  QVariant const& value)
{
    switch (change)
    {
    case GraphicsItemChange::ItemSelectedChange:
        if (!value.toBool())
        {
            proxyWidget->frwd = false;
            if (state == Editing)
            {
                editor->clearFocus();
                editor->setMarkdown(editor->toPlainText());
            }
            state = State::Normal;
        }
        break;
    default:
        break;
    }

    return value;

}

void
CommentGraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        return event->ignore();
    }

    if (state == Editing) return;

    // check for resize handle hit
    bool resize = resizeHandleRect().contains(event->pos());
    if (resize)
    {
        state = Resizing;
        return;
    }

    state = Translating;
    translationDiff = pos();
    setSelected(true);
    event->accept();
}

void
CommentGraphicsObject::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QPointF diff = event->pos() - event->lastPos();
    translationDiff += diff;

    assert(state != Editing);

    switch (state)
    {
    case Resizing:
        event->accept();
        proxyWidget->resize(proxyWidget->size() + QSize{(int)diff.x(), (int)diff.y()});
        break;

    case Translating:
        event->accept();
        moveBy(diff.x(), diff.y());
        break;

    case Normal:
    default:
        event->ignore();
        break;
    }
}

void
CommentGraphicsObject::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    assert(state != Editing);

    switch (state)
    {
    case Resizing:
    case Translating:
        event->accept();
        break;

    case Normal:
    default:
        event->ignore();
        break;
    }
    state = Normal;
}

void
CommentGraphicsObject::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    proxyWidget->frwd = true;
    editor->setPlainText(editor->toMarkdown());
    editor->setFocus();
    state = Editing;
    return QGraphicsObject::mouseDoubleClickEvent(event);
}

void
CommentGraphicsObject::keyPressEvent(QKeyEvent* event)
{
    return QGraphicsObject::keyPressEvent(event);
}

void
CommentGraphicsObject::keyReleaseEvent(QKeyEvent* event)
{
    return QGraphicsObject::keyReleaseEvent(event);
}

void
CommentGraphicsObject::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    hovered = true;
    update();
}

void
CommentGraphicsObject::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    QPointF pos = event->pos();

    if (resizeHandleRect().contains(pos))
    {
        setCursor(QCursor(Qt::SizeFDiagCursor));
        return;
    }

    setCursor(QCursor());
}

void
CommentGraphicsObject::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    hovered = false;
    update();
}

void
CommentGraphicsObject::paint(QPainter* painter,
                             QStyleOptionGraphicsItem const* option,
                             QWidget* widget)
{
    proxyWidget->paint(painter, option, widget);

    auto& style = style::currentStyle().node;
    QPen pen;

    pen.setColor((isSelected())? style.selectedOutline : style.hoveredOutline);
    pen.setWidthF(hovered ? style.hoveredOutlineWidth : style.selectedOutlineWidth);

    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(boundingRect());

    QRectF rect = resizeHandleRect();

    QPolygonF poly;
    poly.append(rect.bottomLeft());
    poly.append(rect.bottomRight());
    poly.append(rect.topRight());

    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::green);
    painter->drawPolygon(poly);
}

