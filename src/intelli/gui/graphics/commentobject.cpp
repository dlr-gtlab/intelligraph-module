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
#include "intelli/gui/graphics/nodeobject.h"
#include <intelli/gui/style.h>
#include <intelli/gui/commentdata.h>
#include <intelli/private/utils.h>

#include <gt_application.h>
#include <gt_icons.h>
#include <gt_colors.h>
#include <gt_palette.h>
#include <gt_guiutilities.h>

#include <QPainter>
#include <QTextEdit>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QRegularExpression>
#include <QGraphicsScene>
#include <QMenu>
#include <QAction>
#include <QVBoxLayout>

using namespace intelli;

/**
 * @brief The CommentGraphicsObject::Overlay class.
 * Helper class that forwards all key, mouse, and hover events it recieves
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
        if (auto anchor = graphics_cast<GraphicsObject*>(p->m_anchor.data()))
        {
            if (anchor->isHovered()) isHovered = true;
        }
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

CommentGraphicsObject::CommentGraphicsObject(QGraphicsScene& scene,
                                             Graph& graph,
                                             CommentData& comment,
                                             GraphSceneData const& data) :
    InteractableGraphicsObject(data, nullptr),
    m_graph(&graph),
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
    m_editor->setMarkdown(m_comment->text());
    m_editor->setMinimumSize(50, 25);

    auto* w = new QWidget();
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_editor);
    w->setLayout(lay);

    m_proxyWidget = new QGraphicsProxyWidget(this);
    m_proxyWidget->setWidget(w);
    m_proxyWidget->setZValue(0);

    m_overlay = new Overlay(this);
    m_overlay->setZValue(1);

    // setup connections
    connect(gtApp, &GtApplication::themeChanged, this, [this, w](){
        gt::gui::applyThemeToWidget(w);
        update();
    });

    connect(this, &GraphicsObject::hoveredChanged, this, [this](){
        if (isHovered()) return setZValue(style::zValue(style::ZValue::NodeHovered));
        if (!isSelected()) return setZValue(style::zValue(style::ZValue::Node));
    });

    connect(this, &InteractableGraphicsObject::objectMoved, this, [this](){
        commitPosition();
    }, Qt::DirectConnection);

    connect(this, &InteractableGraphicsObject::objectCollapsed,
            this, &CommentGraphicsObject::onObjectCollapsed);
    
    connect(m_comment, &CommentData::commentPositionChanged, this, [this](){
        setPos(m_comment->pos());
    }, Qt::DirectConnection);
    
    connect(m_comment, &CommentData::commentCollapsedChanged, this,
            [this](bool doCollapse){
        setCollapsed(doCollapse);
    });

    // setup object
    setPos(m_comment->pos());

    QSize size = m_comment->size();
    if (size.isValid()) m_proxyWidget->widget()->resize(size);

    // setup comment connections
    scene.addItem(this);
    
    for (size_t i = 0; i < m_comment->nNodeConnections(); i++)
    {
        onCommentConnectionAppended(m_comment->nodeConnectionAt(i));
    }
    
    connect(m_comment, &CommentData::nodeConnectionAppended,
            this, &CommentGraphicsObject::onCommentConnectionAppended,
            Qt::DirectConnection);
    
    connect(m_comment, &CommentData::nodeConnectionRemoved,
            this, &CommentGraphicsObject::onCommentConnectionRemoved,
            Qt::DirectConnection);

    collapse(m_comment->isCollapsed());
}

CommentGraphicsObject::~CommentGraphicsObject() = default;

CommentData&
CommentGraphicsObject::commentObject()
{
    assert (m_comment);
    return *m_comment;
}

CommentData const&
CommentGraphicsObject::commentObject() const
{
    return const_cast<CommentGraphicsObject*>(this)->commentObject();
}

ObjectUuid
CommentGraphicsObject::objectUuid() const
{
    return commentObject().uuid();
}

GraphicsObject::DeleteOrdering
CommentGraphicsObject::deleteOrdering() const
{
    return DeleteLast;
}

bool
CommentGraphicsObject::deleteObject()
{
    delete m_comment;
    return true;
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
    if (!isCollapsed() || m_connections.size() != 1) m_comment->setPos(pos());
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
CommentGraphicsObject::setupContextMenu(QMenu& menu)
{
    QAction* connectAction = menu.addAction(tr("Connect to..."));
    connectAction->setIcon(gt::gui::icon::chain());
    connectAction->setVisible(!isCollapsed());

    connect(connectAction, &QAction::triggered, this, [this](){
        auto* scene = this->scene();
        assert(scene);

        auto* draftLine = LineGraphicsObject::makeDraftLine(*this).release();
        scene->addItem(draftLine);
        draftLine->setTypeMask(NodeGraphicsObject::Type);
        draftLine->grabMouse();

        connect(this, &QObject::destroyed,
                draftLine, [draftLine](){ delete draftLine; });

        connect(draftLine, &LineGraphicsObject::finalizeDraftConnection,
                this, [draftLine, this](QGraphicsItem* endItem) mutable {
            delete draftLine;
            auto* nodeItem = graphics_cast<NodeGraphicsObject*>(endItem);
            if (!nodeItem) return;

            auto cmd = gtApp->makeCommand(&commentObject(),
                                          tr("Link comment to %1")
                                              .arg(relativeNodePath(nodeItem->node())));
            Q_UNUSED(cmd);
            
            commentObject().appendNodeConnection(nodeItem->nodeId());
        });
    });

    gt::gui::makeObjectContextMenu(menu, commentObject());
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

    QSize newSize = m_proxyWidget->widget()->size() + diff;

    m_proxyWidget->widget()->resize(newSize);
}

void
CommentGraphicsObject::onCommentConnectionAppended(NodeId nodeId)
{
    if (m_connections.find(nodeId) != m_connections.end()) return;

    assert(scene());

    // find node to connect to
    InteractableGraphicsObject* endItem = nullptr;
    auto const& items = scene()->items();
    for (auto* item : items)
    {
        auto* nodeItem = graphics_cast<NodeGraphicsObject*>(item);
        if (!nodeItem) continue;

        if (nodeItem->nodeId() != nodeId) continue;

        endItem = nodeItem;
        break;
    }

    if (!endItem)
    {
        connect(m_graph.data(), SIGNAL(nodeAppended(Node*)),
                this, SLOT(instantiateMissingConnections()),
                Qt::UniqueConnection);
        return;
    }

    auto lineItem = convert_to_unique_qptr<DirectDeleter>(
        LineGraphicsObject::makeLine(*this, *endItem)
    );

    connect(endItem, &QObject::destroyed, lineItem.get(), [this, nodeId](){
        m_comment->removeNodeConnection(nodeId);
    });
    connect(lineItem, &LineGraphicsObject::deleteRequested, this, [this, nodeId](){
        m_comment->removeNodeConnection(nodeId);
    });

    scene()->addItem(lineItem);
    m_connections.insert({nodeId, std::move(lineItem)});
}

void
CommentGraphicsObject::onCommentConnectionRemoved(NodeId nodeId)
{
    auto iter = m_connections.find(nodeId);
    if (iter == m_connections.end()) return;

    m_connections.erase(iter);
}

void
CommentGraphicsObject::onObjectCollapsed()
{
    m_proxyWidget->setVisible(!isCollapsed());
    m_comment->setCollapsed(isCollapsed());

    if (!isCollapsed() || m_connections.size() != 1)
    {
        if (m_anchor) m_anchor->disconnect(this);
        m_anchor = nullptr;

        setPos(m_comment->pos());
        setInteractionFlag(DefaultInteractionFlags, true);

        for (auto& pair : m_connections)
        {
            LineGraphicsObject* connection = std::get<1>(pair).get();
            assert(connection);
            connection->setVisible(true);
        }
        return;
    }

    LineGraphicsObject* connection = std::get<1>(*m_connections.begin()).get();
    assert(connection);

    connection->setVisible(false);

    auto const* endItem = connection->endItem();
    assert(endItem);

    m_anchor = endItem;

    auto updatePos = [endItem, this]{
        setPos(endItem->pos() +
               endItem->boundingRect().topRight() -
               boundingRect().center());
    };

    setInteractionFlag(DefaultInteractionFlags, false);

    connect(endItem, &QGraphicsObject::xChanged, this, updatePos);
    connect(endItem, &QGraphicsObject::yChanged, this, updatePos);
    updatePos();

    auto graphicsObject = graphics_cast<GraphicsObject*>(endItem);
    if (!graphicsObject) return;

    // trigger update if hovered state of anchored object changed
    connect(graphicsObject, &GraphicsObject::hoveredChanged, this, [this]{
        update();
    });
}

void
CommentGraphicsObject::instantiateMissingConnections()
{
    if (m_comment->nNodeConnections() == m_connections.size())
    {
        m_graph->disconnect(this, SLOT(instantiateMissingConnections()));
    }
    
    for (size_t i = 0; i < m_comment->nNodeConnections(); i++)
    {
        NodeId nodeId = m_comment->nodeConnectionAt(i);
        if (m_connections.find(nodeId) != m_connections.end()) continue;

        // attempt to instantiate missing connection
        onCommentConnectionAppended(nodeId);
    }
}
