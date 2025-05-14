/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/graphics/commentobject.h>
#include <intelli/gui/graphics/lineobject.h>
#include <intelli/gui/style.h>
#include <intelli/gui/commentobject.h>

#include <gt_application.h>
#include <gt_colors.h>
#include <gt_icons.h>

#include <QPainter>
#include <QTextEdit>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QRegularExpression>

using namespace intelli;

/**
 * @brief The CommentGraphicsObject::Overlay class.
 * Helper class that forwards all key, mouse and hover events it recieves
 * to its parent object.
 */
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

        auto& style = style::currentStyle().node;

        bool isSelected = p->isSelected();
        bool isHovered  = p->isHovered();
        QPen pen;
        pen.setColor((isSelected)? style.selectedOutline : style.hoveredOutline);
        pen.setWidthF(isHovered ? style.hoveredOutlineWidth : style.selectedOutlineWidth);

        if (p->isCollapsed())
        {
            QRect rect{QPoint{3, 4}, QSize{24, 24}};
            auto icon = gt::gui::icon::comment();
            painter->setBrush(Qt::black);
            painter->setPen(pen);
            painter->drawEllipse(boundingRect().center(), 15, 15);
            gt::gui::colorize(icon, Qt::white).paint(painter, rect);
            return;
        }

        QRectF resizeRect = p->resizeHandleRect();

        // resize rect
        QPolygonF poly;
        poly.append(resizeRect.bottomLeft());
        poly.append(resizeRect.bottomRight());
        poly.append(resizeRect.topRight());

        painter->setPen(Qt::NoPen);
        painter->setBrush(gt::gui::color::lighten(
            style::currentStyle().node.defaultOutline, -30));
        painter->drawPolygon(poly);

        // outline
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
        event->accept();
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override
    {
        auto* p = static_cast<CommentGraphicsObject*>(parentObject());
        p->mouseMoveEvent(event);
        event->accept();
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
    {
        auto* p = static_cast<CommentGraphicsObject*>(parentObject());
        p->mouseReleaseEvent(event);
        event->accept();
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

CommentGraphicsObject::CommentGraphicsObject(CommentObject& comment,
                                             GraphSceneData const& data) :
    InteractableGraphicsObject(data, nullptr),
    m_comment(&comment)
{
    setFlag(GraphicsItemFlag::ItemIsSelectable, true);
    setFlag(GraphicsItemFlag::ItemContainsChildrenInShape, true);

    setAcceptHoverEvents(true);

    m_editor = new QTextEdit;
    m_editor->setPlaceholderText(tr("Enter comment..."));
    m_editor->setFrameShape(QFrame::NoFrame);
    m_editor->setContextMenuPolicy(Qt::NoContextMenu);
    m_editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_editor->setReadOnly(true);
    m_editor->setMarkdown(comment.text());

    auto* widget = new QGraphicsProxyWidget(this);
    widget->setWidget(m_editor);
    widget->setZValue(0);

    m_overlay = new Overlay(this);
    m_overlay->setZValue(1);

    m_proxyWidget = widget;

    // setup connections
    connect(this, &InteractableGraphicsObject::objectMoved, this, [this](){
        commitPosition();
    }, Qt::DirectConnection);

    connect(this, &InteractableGraphicsObject::objectCollapsed, this, [this](){
        m_proxyWidget->setVisible(!isCollapsed());
        m_comment->setCollapsed(isCollapsed());

        if (isCollapsed() && m_connetions.size() == 1 && m_connetions.front())
        {
            auto const* endItem = m_connetions.front()->endItem();
            assert(endItem);

            m_collapsedAnchor = endItem;

            setPos(endItem->pos() +
                   endItem->boundingRect().topRight() -
                   boundingRect().center());

            setInteractionFlag(DefaultInteractionFlags, false);
            setInteractionFlag(AllowSelecting, true);

            connect(endItem, &QGraphicsObject::xChanged, this, [endItem, this]{
                setPos(endItem->pos() +
                       endItem->boundingRect().topRight() -
                       boundingRect().center());
            });
            connect(endItem, &QGraphicsObject::yChanged, this, [endItem, this]{
                setPos(endItem->pos() +
                       endItem->boundingRect().topRight() -
                       boundingRect().center());
            });
            return;
        }

        if (m_collapsedAnchor) m_collapsedAnchor->disconnect(this);
        m_collapsedAnchor = nullptr;

        setPos(m_comment->pos());
        setInteractionFlag(DefaultInteractionFlags, true);
    }, Qt::DirectConnection);

    connect(m_comment, &CommentObject::commentPositionChanged, this, [this](){
        setPos(m_comment->pos());
    }, Qt::DirectConnection);

    connect(m_comment, &CommentObject::commentCollapsedChanged, this,
            [this](bool doCollapse){
        setCollapsed(doCollapse);
    });

    // setup object
    setPos(m_comment->pos());

    collapse(m_comment->isCollapsed());

    QSize size = m_comment->size();
    if (size.isValid()) m_editor->resize(size);
}

CommentObject&
CommentGraphicsObject::commentObject()
{
    assert (m_comment);
    return *m_comment;
}

CommentObject const&
CommentGraphicsObject::commentObject() const
{
    return const_cast<CommentGraphicsObject*>(this)->commentObject();
}

QRectF
CommentGraphicsObject::boundingRect() const
{
    if (isCollapsed()) return QRectF{QPointF{0, 0}, QSizeF{30, 30}};

    QRectF rect = m_proxyWidget->boundingRect();
    return rect;
}

QRectF
CommentGraphicsObject::widgetSceneBoundingRect() const
{
    return m_proxyWidget->sceneBoundingRect();
}

void
CommentGraphicsObject::startEditing()
{
    unsetCursor();
    setSelected(true);

    m_editor->setReadOnly(false);

    // strip markdown from unnecessary new lines
    static QRegularExpression regexp(
        R"(^\s*?)"         // start of line (inc. spaces)
        R"((\#|\-|\*).+?)" // group 1: line is a heading or list
        R"((\n\n))",       // group 2: double spaces
        QRegularExpression::MultilineOption
    );

    QString markdown = m_editor->toMarkdown();

    QRegularExpressionMatch match = regexp.match(markdown);
    while (match.hasMatch())
    {
        constexpr int group = 2;
        int start = match.capturedStart(group);
        markdown.remove(start, 1);
        match = regexp.match(markdown, start + 1);
    }

    m_editor->setPlainText(markdown);
    m_editor->setFocus();

    auto cursor = m_editor->textCursor();
    cursor.movePosition(QTextCursor::MoveOperation::End);
    m_editor->setTextCursor(cursor);

    m_overlay->setZValue(-1);
}

void
CommentGraphicsObject::finishEditing()
{
    if (m_overlay->zValue() < 0)
    {
        m_editor->clearFocus();
        m_comment->setText(m_editor->toPlainText());
        m_editor->setMarkdown(m_comment->text());
    }

    m_editor->setReadOnly(true);
    m_proxyWidget->unsetCursor();
    m_overlay->setZValue(1);
}

void
CommentGraphicsObject::commitPosition()
{
    if (!isCollapsed()) m_comment->setPos(pos());
}

void
CommentGraphicsObject::addConnection(LineGraphicsObject* line)
{
    if (!line) return;

    m_connetions.append(line);

    connect(line, &QObject::destroyed, this, [this](){
        // remove null connections
        m_connetions.erase(std::remove_if(m_connetions.begin(),
                                          m_connetions.end(),
                                          [](auto const& e){ return !e; }),
                           m_connetions.end());
    }, Qt::QueuedConnection);
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
        break;
    }

    return value;

}

void
CommentGraphicsObject::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    bool isResizing = (state() == State::Resizing);

    InteractableGraphicsObject::mouseReleaseEvent(event);

    if (isResizing)
    {
        QWidget* w = m_proxyWidget->widget();
        m_comment->setSize(w->size());
    }
}

void
CommentGraphicsObject::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    auto const& pos = event->pos();
    emit contextMenuRequested(this, pos);
}

void
CommentGraphicsObject::paint(QPainter* painter,
                             QStyleOptionGraphicsItem const* option,
                             QWidget* widget)
{
    m_proxyWidget->paint(painter, option, widget);
    m_overlay->paint(painter, option, widget);
}

bool
CommentGraphicsObject::canResize(QPointF localCoord)
{
    return !isCollapsed() && resizeHandleRect().contains(localCoord);
}

void
CommentGraphicsObject::resize(QSize diff)
{
    prepareGeometryChange();

    QWidget* w = m_proxyWidget->widget();
    QSize newSize = w->size() + diff;
    QSize minSize = w->minimumSizeHint();

    newSize.rwidth()  = std::max(newSize.width(),  minSize.width());
    newSize.rheight() = std::max(newSize.height(), minSize.height());

    w->resize(newSize);
}

