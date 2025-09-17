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
class CommentData;

class GT_INTELLI_TEST_EXPORT CommentGraphicsObject : public InteractableGraphicsObject
{
    Q_OBJECT

public:

    // Needed for graphics_cast
    enum { Type = make_graphics_type<GraphicsItemType::Comment, InteractableGraphicsObject>() };
    int type() const override { return Type; }

    CommentGraphicsObject(QGraphicsScene& scene,
                          Graph& graph,
                          CommentData& comment,
                          GraphSceneData const& data);
    ~CommentGraphicsObject();

    CommentData& commentObject();
    CommentData const& commentObject() const;

    ObjectUuid objectUuid() const override;

    DeleteOrdering deleteOrdering() const override;

    bool deleteObject() override;

    QRectF boundingRect() const override;

    /**
     * @brief Returns the bounding rect of the main widget in scene-coordianates
     * May return an invalid rect if no widget is available
     * @return Scene bounding rect of the main widget
     */
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

    /**
     * @brief Appends actions for the context menu
     * @param menu Menu
     */
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
    void resizeBy(QSize diff) override;

private:

    class Overlay;

    /// node connections
    std::unordered_map<size_t, unique_qptr<LineGraphicsObject, DirectDeleter>> m_connections;
    /// pointer to Graph
    QPointer<Graph> m_graph;
    /// pointer to comment object
    QPointer<CommentData> m_comment;
    /// anchor object when comment is collapsed. Object is attached to this
    /// anchor object and cannot be moved unless its uncollapsed.
    QPointer<GraphicsObject const> m_anchor;
    /// Main widget
    QGraphicsProxyWidget* m_proxyWidget;
    /// Overlay widget to suppress mouse and key event to the main widget
    Overlay* m_overlay;
    /// Comment editor
    QTextEdit* m_editor;

    QRectF resizeHandleRect() const;

private slots:

    void onCommentConnectionAppended(NodeId nodeId);

    void onCommentConnectionRemoved(NodeId nodeId);

    void onObjectCollapsed();

    void instantiateMissingConnections();
};

} // namespace intelli

#endif // GT_INTELLI_COMMENTGRAPHICSOBJECT_H
