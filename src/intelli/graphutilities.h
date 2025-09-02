/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHUTILITIES_H
#define GT_INTELLI_GRAPHUTILITIES_H

#include <intelli/exports.h>
#include <intelli/globals.h>
#include <intelli/view.h>

namespace intelli
{

class Graph;

namespace utils
{

/**
 * @brief Copies the objects given by their uuids from the source graph to the
 * target graph. The original objects are not deleted. The selected objects
 * may include nodes and comments. All connections inbetween nodes are copied
 * even if they are not selected. Copied objects are assigned a new UUID and
 * nodes may be assigned new ids.
 *
 * @param source Source graph. Selected objects are only copied and remain in
 * the source graph.
 * @param selection Vector-like list of uuids t o copy
 * @param target Target graph
 * @return Success
 */
GT_INTELLI_EXPORT
bool copyObjectsToGraph(Graph const& source, View<ObjectUuid> selection, Graph& target);

/**
 * @brief Overload that copies all objects from the source graph to the target
 * graph.
 *
 * @param source Source graph. Objects are only copied and remain in
 * the source graph.
 * @param target Target graph
 * @return Success
 */
GT_INTELLI_EXPORT
bool copyObjectsToGraph(Graph const& source, Graph& target);

/**
 * @brief Moves the objects given by their uuids from the source graph to the
 * target graph. The selected objects may include nodes and comments.
 * All connections inbetween nodes are copied even if they are not selected.
 * Node's may be assigned new ids.
 *
 * NOTE: Node objects are not deleted, but connections and comments may be
 * deleted and reinstantiated in the target graph instead.
 *
 * @param source Source graph. Objects are moved and thus do not remain in
 * the source graph.
 * @param selection Vector-like list of uuids to copy
 * @param target Target graph
 * @return Success
 */
GT_INTELLI_EXPORT
bool moveObjectsToGraph(Graph& source, View<ObjectUuid> selection, Graph& target);

/**
 * @brief Overload that moves all objects from the source graph to the target
 * graph.
 * @param source Source graph. Objects are moved and thus do not remain in
 * the source graph.
 * @param target Target graph
 * @return Success
 */
GT_INTELLI_EXPORT
bool moveObjectsToGraph(Graph& source, Graph& target);

/**
 * @brief Groups the objects given by their uuids into a subgraph node
 * (= group node) as a child of the source graph. The subgraph node will be
 * created and named according to `targetCaption`. The selected objects will
 * be moved into the subgraph node according to `moveObjectsToGraph`. The
 * selected objects may include nodes and comments.
 * @param source Source graph. The selection must only contain objects belonging
 * to this graph.
 * @param targetCaption The name/caption of the subgraph
 * @param selection Selection to move to subgraph
 * @return Pointer to newly created subgraph node. Will return null if the
 * operation failed for any reason.
 */
GT_INTELLI_EXPORT
Graph* groupObjects(Graph& source,
                    QString const& targetCaption,
                    View<ObjectUuid> selection);

/**
 * @brief Expands the given subgraph. All nodes, connections, and comments will
 * be expanded into the parent graph, according to `moveObjectsToGraph`.
 * Subgraph is deleted once its expanded.
 * @param groupNode. Pointer to subgrah. Must no be null. Subgraph will be
 * deleted and thus any reference/pointer is invalidated.
 * @return Success
 */
GT_INTELLI_EXPORT
bool expandSubgraph(std::unique_ptr<Graph> groupNode);

/**
 * @brief Duplicates the source graph and inserts the new graph as a sibling
 * @param source Source graph to duplicate
 * @return Duplicated graph (may be null if operation failed)
 */
GT_INTELLI_EXPORT
Graph* duplicateGraph(Graph& source);

} // namespace utils

} // namespace intelli

#endif // GT_INTELLI_GRAPHUTILITIES_H
