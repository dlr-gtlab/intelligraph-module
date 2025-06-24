/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_COMMENTGRAPHICSOBJECT_H
#define GT_INTELLI_COMMENTGRAPHICSOBJECT_H

#include <intelli/globals.h>
#include <intelli/memory.h>
#include <intelli/gui/graphics/interactableobject.h>

#include <unordered_map>

class QGraphicsProxyWidget;
class QTextEdit;

namespace intelli
{

class Graph;
class LineGraphicsObject;
class CommentObject;

class GT_INTELLI_TEST_EXPORT CommentGraphicsObject : public InteractableGraphicsObject
{
    Q_OBJECT

public:

    // Needed for graphics_cast
    enum { Type = make_graphics_type<GraphicsItemType::Comment, InteractableGraphicsObject>() };
    int type() const override { return Type; }

    CommentGraphicsObject(QGraphicsScene& scene,
                          Graph& graph,
                          CommentObject& comment,
                          GraphSceneData const& data);
    ~CommentGraphicsObject();

    CommentObject& commentObject();
    CommentObject const& commentObject() const;

    DeleteOrdering deleteOrdering() const override;

    bool deleteObject() override;

    QRectF boundingRect() const override;

    QRectF widgetSceneBoundingRect() const override;

    /**
     * @brief Starts editing the comment (makes the text edit editable)
     */
    void startEditing();

    /**
     * @brief Exits editing the comment (makes the text edit uneditable)
     */
    void finishEditing();

    /**
     * @brief Commits the position of this object to the associated node
     */
    void commitPosition() override;

    void setupContextMenu(QMenu& menu) override;

protected:

    void paint(QPainter *painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

    QVariant itemChange(GraphicsItemChange ch5ange, QVariant const& value) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

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

private:

    class Overlay;

    std::unordered_map<ObjectUuid, unique_qptr<LineGraphicsObject, DirectDeleter>> m_connections;
    QPointer<Graph> m_graph;
    /// pointer to comment object
    QPointer<CommentObject> m_comment;
    /// anchor object when comment is collapsed. Object is attached to this
    /// anchor object and cannot be moved unless its uncollapsed.
    QPointer<GraphicsObject const> m_anchor;
    /// Main widget
    QGraphicsProxyWidget* m_proxyWidget;
    /// Overlay widget to supress mouse and key event to the main widget
    Overlay* m_overlay;
    /// Comment editor
    QTextEdit* m_editor;

    QRectF resizeHandleRect() const;

private slots:

    void onCommentConnectionAppended(ObjectUuid const& objectUuid);

    void onCommentConnectionRemoved(ObjectUuid const& objectUuid);

    void onObjectCollapsed();

    void instantiateMissingConnections();
};

} // namespace intelli

#endif // GT_INTELLI_COMMENTGRAPHICSOBJECT_H
