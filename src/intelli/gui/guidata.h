/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GUIDATA_H
#define GT_INTELLI_GUIDATA_H

#include <intelli/globals.h>

#include <gt_object.h>
#include <gt_propertystructcontainer.h>

namespace intelli
{

class Graph;
class CommentGroup;
class LocalStateContainer;

/**
 * @brief The GuiData class. Base object for GUI-specific data
 */
class GuiData : public GtObject
{
    Q_OBJECT

public:

    Q_INVOKABLE GuiData(GtObject* parent = nullptr);

    static LocalStateContainer* accessLocalStates(Graph& graph);
    static LocalStateContainer const* accessLocalStates(Graph const& graph);

    static CommentGroup* accessCommentGroup(Graph& graph);
    static CommentGroup const* accessCommentGroup(Graph const& graph);
};

/**
 * @brief The LocalStateContainer class. Data object for storing states
 * specific to a graph.
 */
class LocalStateContainer : public GtObject
{
    Q_OBJECT

public:

    Q_INVOKABLE LocalStateContainer(GtObject* parent = nullptr);

    /**
     * @brief Sets the collapsed state for the given node.
     * @param nodeUuid Node's Uuid
     * @param collapsed Whether the node is collapsed or expanded
     */
    void setNodeCollapsed(NodeUuid const& nodeUuid, bool collapsed = true);

    /**
     * @brief Returns whether the node is collapsed or expanded
     * @param nodeUuid Node's Uuid
     * @return Whether the node is collapsed or expanded
     */
    bool isNodeCollapsed(NodeUuid const& nodeUuid) const;

signals:

    /**
     * @brief Emitted once the node changes its collapsed state
     * @param nodeUuid Node's Uuid
     * @param isCollapsed Whether node is collapsed or not
     */
    void nodeCollapsedChanged(QString const& nodeUuid, bool isCollapsed);

private:

    /// Struct container for storing all nodes that are collapsed.
    /// Nodes that are not present are expanded.
    GtPropertyStructContainer m_collapsed;

    /// TODO: remove me once core issue #1366 is merged
    QStringList m_collapsedData;
};

} // namespace intelli

#endif // GT_INTELLI_GUIDATA_H
