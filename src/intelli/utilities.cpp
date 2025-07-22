/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/utilities.h>

#include <intelli/graph.h>
#include <intelli/gui/guidata.h>
#include <intelli/gui/commentgroup.h>
#include <intelli/gui/commentdata.h>
#include <intelli/private/utils.h>

template<typename T>
using has_gt_log_call_operator =
    std::enable_if_t<
        std::is_same<
            decltype(std::declval<T const&>()
                         .operator()(std::declval<gt::log::Stream&>())),
            gt::log::Stream&>::value,
        bool>;

template <typename T, has_gt_log_call_operator<T> = true>
inline gt::log::Stream& operator<<(gt::log::Stream& s, T const& f)
{
    return f(s);
}

template <typename T>
struct Expected
{
    T value{};
    bool success{};

    operator bool() const { return success; }

    T operator *() { return value; }
    T const operator *() const { return value; }

    T operator ->() { return value; }
    T const operator ->() const { return value; }
};

using namespace intelli;

namespace
{

/// creates a copy of the object and returns a unique_ptr
template <typename T>
inline auto makeCopy(T const& obj)
{
    std::unique_ptr<GtObject> tmp{obj.copy()};
    return gt::unique_qobject_cast<T>(std::move(tmp));
}

template <typename LogFunc>
inline Expected<Node*>
copyNodeToGraph(Graph const& source,
                Node const& node,
                Graph& target,
                LogFunc const& makeError)
{
    if (node.nodeFlags() & NodeFlag::Unique) return {nullptr, true};
    if (!(node.objectFlags() & GtObject::UserDeletable)) return {nullptr, true};

    Node* copiedNode = target.appendNode(makeCopy(node));
    if (!copiedNode)
    {
        gtError() << makeError
                  << QObject::tr("Failed to copy node '%1'")
                         .arg(relativeNodePath(node));
        return {};
    }

    return {copiedNode, true};
}

template <typename LogFunc>
inline Expected<CommentData*>
copyCommentToGraph(CommentData const& comment,
                   CommentGroup& targetCommentGroup,
                   LogFunc const& makeError)
{
    CommentData* copiedComment = targetCommentGroup.appendComment(makeCopy(comment));
    if (!copiedComment)
    {
        gtError() << makeError
                  << QObject::tr("Failed to append comment '%1'")
                         .arg(comment.objectName());
        return {};
    }

    return {copiedComment, true};
}

bool
copyObjectsToGraph(Graph const& source,
                   View<Node const*> nodes,
                   View<CommentData const*> comments,
                   Graph& target)
{
    auto const makeError = [&source, &target](gt::log::Stream& s) -> gt::log::Stream& {
        return s << utils::logId(source)
                 << QObject::tr("Error copying objects to '%2':")
                        .arg(relativeNodePath(target));
    };

    auto targetChangeCmd = target.modify();
    Q_UNUSED(targetChangeCmd);

    // maps original node ids to updated node ids
    QMap<NodeId, NodeId> changedNodeIds;

    // find internal connections
    QVector<ConnectionUuid> internalConnections;
    for (Node const* node : nodes)
    {
        assert(node);
        NodeId nodeId = node->id();

        auto& conModel = source.connectionModel();
        auto conections = conModel.iterateConnections(nodeId, PortType::Out);
        for (ConnectionId conId : conections)
        {
            assert(conId.outNodeId == nodeId);
            if (!containsNodeId(conId.inNodeId, nodes)) continue;

            internalConnections.append(source.connectionUuid(conId));
        }
    }

    // copy nodes and update connections
    for (Node const* sourceNode : nodes)
    {
        assert(sourceNode);

        auto copiedNode = copyNodeToGraph(source, *sourceNode, target, makeError);
        if (!copiedNode.success) return false;
        if (!copiedNode.value) continue;

        auto affectedCons = std::partition(internalConnections.begin(),
                                           internalConnections.end(),
                                           [nodeUuid = sourceNode->uuid()]
                                           (ConnectionUuid const& connection){
            assert(connection.outNodeId != connection.inNodeId);
            return connection.outNodeId == nodeUuid ||
                   connection.inNodeId == nodeUuid;
        });

        for (ConnectionUuid& connection : makeIterable(internalConnections.begin(),
                                                       affectedCons))
        {
            (connection.outNodeId == sourceNode->uuid() ?
                 connection.outNodeId :
                 connection.inNodeId) = copiedNode->uuid();
        }

        changedNodeIds.insert(sourceNode->id(), copiedNode->id());
    }

    // append updated connections
    for (ConnectionUuid const& conUuid : internalConnections)
    {
        ConnectionId conId = target.connectionId(conUuid);
        if (!conId.isValid())
        {
            gtError() << makeError
                      << QObject::tr("Failed to resolve connection '%1'!")
                             .arg(toString(conUuid));
            continue;
        }

        if (!target.appendConnection(conId))
        {
            gtError() << makeError
                      << QObject::tr("Failed to append connection '%1'")
                             .arg(toString(conId));
            return false;
        }
    }

    // append comments
    CommentGroup const* commentGroup = GuiData::accessCommentGroup(source);
    assert(commentGroup);
    CommentGroup* targetCommentGroup = GuiData::accessCommentGroup(target);
    assert(targetCommentGroup);
    for (CommentData const* sourceComment : comments)
    {
        assert(sourceComment);

        auto copiedComment = copyCommentToGraph(*sourceComment,
                                                *targetCommentGroup,
                                                makeError);
        if (!copiedComment) return false;

        // update node connections
        size_t idx  = 0;
        size_t size = copiedComment->nNodeConnections();
        while (idx < size)
        {
            NodeId nodeId = copiedComment->nodeConnectionAt(idx);

            auto iter = changedNodeIds.find(nodeId);
            if (iter == changedNodeIds.end()) // connected node was not copied
            {
                copiedComment->removeNodeConnection(nodeId);
                size--;
                continue;
            }

            if (nodeId == iter.value()) // node id has not changed
            {
                idx++;
                continue;
            }

            // update node ids
            copiedComment->removeNodeConnection(nodeId);
            copiedComment->appendNodeConnection(iter.value());
            size--;
        }
    }

    return true;
}

bool
moveObjectsToGraph(Graph& source,
                   View<Node const*> nodes,
                   View<CommentData*> comments,
                   Graph& target)
{
    auto const makeError = [&source, &target](gt::log::Stream& s) -> gt::log::Stream& {
        return s << utils::logId(source)
                 << QObject::tr("Error moving objects to '%2':")
                        .arg(relativeNodePath(target));
    };

    auto sourceChangeCmd = source.modify();
    Q_UNUSED(sourceChangeCmd);
    auto targetChangeCmd = target.modify();
    Q_UNUSED(targetChangeCmd);

    // remeber original node ids
    QVector<NodeId> originalNodeIds;
    originalNodeIds.reserve(nodes.size());
    std::transform(nodes.begin(),
                   nodes.end(),
                   std::back_inserter(originalNodeIds),
                   get_node_id<NodeId>{});
    assert(nodes.size() == (size_t)originalNodeIds.size());

    // move nodes and connections
    bool success = source.moveNodesAndConnections(nodes, target);
    if (!success)
    {
        gtError() << makeError
                  << QObject::tr("Failed to move nodes");
        return false;
    }

    // move comments
    CommentGroup* sourceCommentGroup = GuiData::accessCommentGroup(source);
    assert(sourceCommentGroup);
    CommentGroup* targetCommentGroup = GuiData::accessCommentGroup(target);
    assert(targetCommentGroup);
    for (CommentData* sourceComment : comments)
    {
        assert(sourceComment);

        auto copiedComment = copyCommentToGraph(*sourceComment, *targetCommentGroup, makeError);
        if (!copiedComment) return false;

        delete sourceComment;
        sourceComment = nullptr;

        // update node connections
        size_t idx  = 0;
        size_t size = copiedComment->nNodeConnections();
        while (idx < size)
        {
            NodeId nodeId = copiedComment->nodeConnectionAt(idx);

            auto iter = std::find(originalNodeIds.begin(), originalNodeIds.end(), nodeId);
            if (iter == originalNodeIds.end()) // connected node was not moved
            {
                copiedComment->removeNodeConnection(nodeId);
                size--;
                continue;
            }

            size_t idx = std::distance(originalNodeIds.begin(), iter);
            assert(idx < nodes.size());
            Node const* node = nodes.at(idx);
            if (nodeId == node->id()) // node id has not changed
            {
                idx++;
                continue;
            }

            // update node ids
            copiedComment->removeNodeConnection(nodeId);
            copiedComment->appendNodeConnection(node->id());
            size--;
        }
    }

    return true;
}

} // namespace

bool
intelli::utils::copyObjectsToGraph(Graph const& source,
                                   View<ObjectUuid> selection,
                                   Graph& target)
{
    QVector<Node const*> nodes;
    QVector<CommentData const*> comments;

    CommentGroup const* commentGroup = GuiData::accessCommentGroup(source);
    assert(commentGroup);

    // copy objects over
    for (ObjectUuid const& uuid : selection)
    {
        if (Node const* node = source.findNodeByUuid(uuid))
        {
            if (Graph::accessGraph(*node) != &source) continue;

            nodes.append(node);
            continue;
        }

        if (auto const* comment = qobject_cast<CommentData const*>(commentGroup->getObjectByUuid(uuid)))
        {
            comments.append(comment);
            continue;
        }
    }

    return ::copyObjectsToGraph(source, nodes, comments, target);
}

bool
intelli::utils::copyObjectsToGraph(Graph const& source,
                                   Graph& target)
{
    auto nodeIter = source.connectionModel().iterateNodes();

    QVector<Node const*> nodes;
    nodes.reserve(nodeIter.size());
    std::copy(nodeIter.begin(), nodeIter.end(), std::back_inserter(nodes));

    CommentGroup const* commentGroup = GuiData::accessCommentGroup(source);
    assert(commentGroup);
    QVector<CommentData const*> comments = commentGroup->comments().toVector();

    return ::copyObjectsToGraph(source, nodes, comments, target);
}

bool
intelli::utils::moveObjectsToGraph(Graph& source, View<ObjectUuid> selection, Graph& target)
{
    QVector<Node*> nodes;
    QVector<CommentData*> comments;

    CommentGroup* commentGroup = GuiData::accessCommentGroup(source);
    assert(commentGroup);

    // copy objects over
    for (ObjectUuid const& uuid : selection)
    {
        if (Node* node = source.findNodeByUuid(uuid))
        {
            if (Graph::accessGraph(*node) != &source) continue;

            nodes.append(node);
            continue;
        }

        if (auto* comment = qobject_cast<CommentData*>(commentGroup->getObjectByUuid(uuid)))
        {
            comments.append(comment);
            continue;
        }
    }

    return ::moveObjectsToGraph(source, nodes, comments, target);
}

bool
intelli::utils::moveObjectsToGraph(Graph& source, Graph& target)
{
    auto nodeIter = source.connectionModel().iterateNodes();

    QVector<Node const*> nodes;
    nodes.reserve(nodeIter.size());
    std::copy(nodeIter.begin(), nodeIter.end(), std::back_inserter(nodes));

    CommentGroup* commentGroup = GuiData::accessCommentGroup(source);
    assert(commentGroup);
    QVector<CommentData*> comments = commentGroup->comments().toVector();

    return ::moveObjectsToGraph(source, nodes, comments, target);
}
