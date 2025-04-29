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


class CommentGraphicsObject::Overlay : public QGraphicsObject
{
public:

    Overlay(QGraphicsObject* parent = nullptr) : QGraphicsObject(parent)
    {
        setAcceptHoverEvents(true);
    }

    QRectF boundingRect() const override
    {
        return static_cast<CommentGraphicsObject*>(parentObject())->boundingRect();
    }

    void paint(QPainter *painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override
    {
        auto* p = static_cast<CommentGraphicsObject*>(parentObject());
        QRectF rect = p->resizeHandleRect();

        auto& style = style::currentStyle().node;

        // resize rect
        QPolygonF poly;
        poly.append(rect.bottomLeft());
        poly.append(rect.bottomRight());
        poly.append(rect.topRight());

        painter->setPen(Qt::NoPen);
        painter->setBrush(gt::gui::color::lighten(
            style::currentStyle().node.defaultOutline, -30));
        painter->drawPolygon(poly);

        // outline
        QPen pen;
        pen.setColor((p->isSelected())? style.selectedOutline : style.hoveredOutline);
        pen.setWidthF(p->isHovered() ? style.hoveredOutlineWidth : style.selectedOutlineWidth);

        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
    }

protected:

    void keyPressEvent(QKeyEvent* event) override
    {
        event->accept();
    }

    void keyReleaseEvent(QKeyEvent* event) override
    {
        event->accept();
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override
    {
        auto* p = static_cast<CommentGraphicsObject*>(parentObject());
        p->mousePressEvent(event);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override
    {
        auto* p = static_cast<CommentGraphicsObject*>(parentObject());
        p->mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
    {
        auto* p = static_cast<CommentGraphicsObject*>(parentObject());
        p->mouseReleaseEvent(event);
    }

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override
    {
        auto* p = static_cast<CommentGraphicsObject*>(parentObject());
        p->startEditing();
        event->accept();
    }

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override
    {
        auto* p = static_cast<CommentGraphicsObject*>(parentObject());
        p->hoverEnterEvent(event);
    }

    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override
    {
        auto* p = static_cast<CommentGraphicsObject*>(parentObject());
        p->hoverMoveEvent(event);
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override
    {
        auto* p = static_cast<CommentGraphicsObject*>(parentObject());
        p->hoverLeaveEvent(event);
    }
};

CommentGraphicsObject::CommentGraphicsObject(GraphSceneData const& data) :
    InteractableGraphicsObject(data, nullptr)
{
    setFlag(GraphicsItemFlag::ItemIsSelectable, true);

    setAcceptHoverEvents(true);

    editor = new QTextEdit;
    editor->setMarkdown(tr("# Hello World\n\nthis is some text\n\n`this is code`\n"));
    editor->setFrameShape(QFrame::NoFrame);
    editor->setContextMenuPolicy(Qt::NoContextMenu);
    editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* widget = new QGraphicsProxyWidget(this);
    widget->setWidget(editor);
    widget->setZValue(0);

    overlay = new Overlay(this);
    overlay->setZValue(1);

    proxyWidget = widget;
}

QRectF
CommentGraphicsObject::boundingRect() const
{
    QRectF rect = proxyWidget->boundingRect();
    return rect;
}

void
CommentGraphicsObject::startEditing()
{
    unsetCursor();

    editor->setPlainText(editor->toMarkdown());
    editor->setFocus();

    auto cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::MoveOperation::End);
    editor->setTextCursor(cursor);

    overlay->setZValue(-1);
}

void
CommentGraphicsObject::finishEditing()
{
    if (overlay->zValue() < 0)
    {
        editor->clearFocus();
        editor->setMarkdown(editor->toPlainText());
    }

    proxyWidget->unsetCursor();
    overlay->setZValue(1);
}

QRectF
CommentGraphicsObject::resizeHandleRect() const
{
    constexpr QSize size{8, 8};

    QRectF body = boundingRect();
    return QRectF(body.bottomRight() - QPoint{size.width(), size.height()}, size);
}

QVariant
CommentGraphicsObject::itemChange(GraphicsItemChange change, QVariant const& value)
{
    switch (change)
    {
    case GraphicsItemChange::ItemSelectedChange:
    {
        bool isSelected = value.toBool();
        if (!isSelected) finishEditing();
        break;
    }
    default:
        return QGraphicsObject::itemChange(change, value);
    }

    return value;

}

void
CommentGraphicsObject::paint(QPainter* painter,
                             QStyleOptionGraphicsItem const* option,
                             QWidget* widget)
{
    proxyWidget->paint(painter, option, widget);
    overlay->paint(painter, option, widget);
}

bool
CommentGraphicsObject::canResize(QPointF localCoord)
{
    return resizeHandleRect().contains(localCoord);
}

void
CommentGraphicsObject::resize(QSize diff)
{
    QWidget* w = proxyWidget->widget();
    QSize newSize = w->size() + diff;
    QSize minSize = w->minimumSizeHint();

    newSize.rwidth()  = std::max(newSize.width(),  minSize.width());
    newSize.rheight() = std::max(newSize.height(), minSize.height());

    w->resize(newSize);
}

