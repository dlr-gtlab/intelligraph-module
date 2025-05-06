/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_COMMENTGRAPHICSOBJECT_H
#define GT_INTELLI_COMMENTGRAPHICSOBJECT_H

#include <intelli/globals.h>
#include <intelli/gui/graphics/interactableobject.h>

#include <QGraphicsObject>
#include <QPointer>

class QGraphicsProxyWidget;
class QTextEdit;

namespace intelli
{

class CommentObject;
class CommentGraphicsObject : public InteractableGraphicsObject
{
    Q_OBJECT

public:

    // Needed for qgraphicsitem_cast
    enum { Type = UserType + (int)GraphicsItemType::N_ENTRIES + 1 };
    int type() const override { return Type; }

    CommentGraphicsObject(CommentObject& comment, GraphSceneData const& data);

    CommentObject& commentObject();
    CommentObject const& commentObject() const;

    QRectF boundingRect() const override;

    /**
     * @brief Starts editing the comment (makes the text edit editable)
     */
    void startEditing();

    /**
     * @brief Exits editing the comment (makes the text edit uneditable)
     */
    void finishEditing();

    void commitPosition();
protected:

    void paint(QPainter *painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

    QVariant itemChange(GraphicsItemChange change, QVariant const& value) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    /**
     * @brief Whether the object should start resizing.
     * @param localCoord Position of cursor within the graphics object.
     * Coordinates may be used to check if mouse hovers over a resize rect or
     * similar.
     * @return Whether the object should start resizing
     */
    bool canResize(QPointF localCoord) override;

    /**
     * @brief Performs the resize action given the size difference.
     * @param diff Difference in size
     */
    void resize(QSize diff) override;

signals:

    /**
     * @brief Emitted once the context menu of a comment was requested
     * @param object Object for which the context menu was requested (this)
     * @param pos Local cursor position
     */
    void contextMenuRequested(CommentGraphicsObject* object, QPointF pos);

private:

    class Overlay;

    QRectF resizeHandleRect() const;

    QPointer<CommentObject> m_comment;
    QGraphicsProxyWidget* proxyWidget;
    Overlay* overlay;
    QTextEdit* editor;
};

} // namespace intelli

#endif // GT_INTELLI_COMMENTGRAPHICSOBJECT_H
