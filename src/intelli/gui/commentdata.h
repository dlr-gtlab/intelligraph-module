/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_COMMENTDATA_H
#define GT_INTELLI_COMMENTDATA_H

#include <intelli/globals.h>
#include <intelli/exports.h>

#include <gt_object.h>

namespace intelli
{

/**
 * @brief The CommentData class. Data object for comments that may be
 * linked to nodes.
 */
class GT_INTELLI_TEST_EXPORT CommentData : public GtObject
{
    Q_OBJECT

public:

    Q_INVOKABLE CommentData(GtObject* parent = nullptr);
    ~CommentData();

    /**
     * @brief Sets the text of the comment.
     * @param text Text/comment
     */
    void setText(QString text);

    /**
     * @brief Returns the text of the comment
     * @return Text
     */
    QString const& text() const;

    /**
     * @brief Sets the position of the comment in the graph scene
     * @param pos New position
     */
    void setPos(Position pos);

    /**
     * @brief Returns the position of the comment in a graph scene
     * @return Position
     */
    Position pos() const;

    /**
     * @brief Sets the size of the comment/widget
     * @param size Size of the comment in a graph scene
     */
    void setSize(QSize size);

    /**
     * @brief Returns the size of a comment in a graph scene.
     * @return Size
     */
    QSize size() const;

    /**
     * @brief Sets whether the comment is collapsed or not. A collapsed comment
     * does not display the text of the comment and reduces its size to a
     * minimum.
     * @param collapse Whether the comment is collapsed.
     */
    void setCollapsed(bool collapse);

    /**
     * @brief Returns whether the comment is collapsed
     * @return Whether comment is collapsed
     */
    bool isCollapsed() const;

    /**
     * @brief Links the comment to a node with the id `targetNodeId`. A comment
     * may be linked to none, one, or multiple comments.
     *
     * Note: Does not check if node with the id actually exists.
     *
     * @param targetNodeId Id of the node to link this comment to.
     */
    void appendNodeConnection(NodeId targetNodeId);

    /**
     * @brief Removes the link to the node given its id. If no connection
     * existed, no action is performed and `false` is returned.
     * @param targetNodeId Id of the node that should be unlinked.
     * @return Whether removing the connection succeeded.
     */
    bool removeNodeConnection(NodeId targetNodeId);

    /**
     * @brief Whether the comment has a connection to a node given its node id.
     * @param targetNodeId Id of the node to check
     * @return Whether the comment is linked to the given node.
     */
    bool isNodeConnected(NodeId targetNodeId);

    /**
     * @brief returns the number of nodes this comment is associated with
     * @return Number of connections.
     */
    size_t nNodeConnections() const;

    /**
     * @brief Returns the node id of the connection at index `idx`. The index
     * must be valid i.e. < `nNodeConnections`.
     * @param idx Index of the connection.
     * @return Node id of the connection at index `idx`.
     */
    NodeId nodeConnectionAt(size_t idx) const;

signals:

    /// Emitted once the comment is about to be deleted. Its data can still be
    /// accessed at this moment.
    void aboutToBeDeleted();

    /// Emitted once the collapsed flag changes
    void commentCollapsedChanged(bool collapsed);

    /// Emitted once the position changes (is also triggered by `setPos`)
    void commentPositionChanged();

    /// Emitted once a new node connection was added
    void nodeConnectionAppended(NodeId nodeId);

    /// Emitted once a node connection was removed.
    void nodeConnectionRemoved(NodeId nodeId);

protected:

    void onObjectDataMerged() override;

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace intelli

#endif // GT_INTELLI_COMMENTDATA_H
