/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/graphutilities.h>

#include <intelli/graph.h>
#include <intelli/nodedatafactory.h>
#include <intelli/utilities.h>
#include <intelli/node/groupinputprovider.h>
#include <intelli/node/groupoutputprovider.h>
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
template <typename SourceGraph, typename NodeVec, typename CommentVec>
inline auto
resolveSelection(SourceGraph& sourceGraph,
                 View<ObjectUuid> selection,
                 NodeVec& nodes,
                 CommentVec& comments)
{
    constexpr bool IsConst = is_const<SourceGraph>::value;

    auto* commentGroup = GuiData::accessCommentGroup(sourceGraph);
    assert(commentGroup);

    for (ObjectUuid const& uuid : selection)
    {
        if (auto* node = sourceGraph.findNodeByUuid(uuid))
        {
            if (Graph::accessGraph(*node) != &sourceGraph) continue;

            nodes.append(node);
            continue;
        }

        if (auto* comment = qobject_cast<apply_const_t<IsConst, CommentData>*>(
                commentGroup->getObjectByUuid(uuid)))
        {
            comments.append(comment);
            continue;
        }
    }
}

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

        for (ConnectionUuid& connection : utils::makeIterable(internalConnections.begin(),
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

} // namespace

bool
intelli::utils::copyObjectsToGraph(Graph const& source,
                                   View<ObjectUuid> selection,
                                   Graph& target)
{
    QVector<Node const*> nodes;
    QVector<CommentData const*> comments;

    resolveSelection(source, selection, nodes, comments);

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

namespace
{

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

            size_t pos = std::distance(originalNodeIds.begin(), iter);
            assert(pos < nodes.size());
            Node const* node = nodes.at(pos);
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
intelli::utils::moveObjectsToGraph(Graph& source, View<ObjectUuid> selection, Graph& target)
{
    QVector<Node*> nodes;
    QVector<CommentData*> comments;

    resolveSelection(source, selection, nodes, comments);

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

namespace
{

Graph* groupObjects(Graph& source,
                    QString const& targetCaption,
                    View<Node*> nodes,
                    View<CommentData*> comments)
{
    auto const makeError = [&source](gt::log::Stream& s) -> gt::log::Stream& {
        return s << utils::logId(source)
                 << QObject::tr("Failed to group objects:");
    };

    QVector<ConnectionUuid> connectionsIn, connectionsOut;

    auto& conModel = source.connectionModel();

    // separate connections into ingoing and outgoing of the group node
    for (Node const* node : nodes)
    {
        for (ConnectionId conId : conModel.iterateConnections(node->id()))
        {
            if (!containsNodeId(conId.inNodeId, nodes))
            {
                connectionsOut.push_back(source.connectionUuid(conId));
            }
            if (!containsNodeId(conId.outNodeId, nodes))
            {
                connectionsIn.push_back(source.connectionUuid(conId));
            }
        }
    }

    // sort in and out going connections to avoid crossing connections
    auto const sortByEndPoint = [&source](ConnectionUuid const& a,
                                          ConnectionUuid const& b){
        constexpr PortType type = PortType::In;
        auto cA = source.findNodeByUuid(a.node(type));
        auto cB = source.findNodeByUuid(b.node(type));
        assert(cA && Graph::accessGraph(*cA) == &source);
        assert(cB && Graph::accessGraph(*cB) == &source);

        // first sort by y pos
        return (cA->pos().y() < cB->pos().y()) ? true :
                   (cA->pos().y() > cB->pos().y()) ? false :
                        // then sort by idx
                        cA->portIndex(type, a.port(type)) <
                        cB->portIndex(type, b.port(type));
    };

    std::sort(connectionsIn.begin(),  connectionsIn.end(),  sortByEndPoint);
    std::sort(connectionsOut.begin(), connectionsOut.end(), sortByEndPoint);

    auto modifyCmd = source.modify();
    Q_UNUSED(modifyCmd);

    // create group node
    auto targetGraphPtr = std::make_unique<Graph>();
    targetGraphPtr->setCaption(targetCaption);

    // setup input/output provider
    targetGraphPtr->initInputOutputProviders();
    auto* inputProvider  = targetGraphPtr->inputProvider();
    auto* outputProvider = targetGraphPtr->outputProvider();

    if (!inputProvider || !outputProvider)
    {
        gtError() << makeError
                  << QObject::tr("Invalid input or output provider!");
        return {};
    }

    // update node positions
    QPolygonF selectionPoly;
    std::transform(nodes.begin(), nodes.end(),
                   std::back_inserter(selectionPoly), [](auto const* node){
        return node->pos();
    });

    auto boundingRect = selectionPoly.boundingRect();
    auto center = boundingRect.center();
    auto offset = QPointF{boundingRect.width() * 0.5,
                          boundingRect.height() * 0.5};

    targetGraphPtr->setPos(center);
    inputProvider->setPos(inputProvider->pos() + center - 2 * offset);
    outputProvider->setPos(outputProvider->pos() + center);

    for (Node* node : qAsConst(nodes))
    {
        node->setPos(node->pos() - offset);
    }

    // find connections that share the same outgoing node and port
    auto const extractSharedConnections = [](auto& connections){
        QVector<ConnectionUuid> shared;

        for (auto begin = connections.begin(); begin != connections.end(); ++begin)
        {
            while (true)
            {
                auto iter = std::find_if(std::next(begin, 1), connections.end(),
                                         [conId = *begin](ConnectionUuid const& other){
                                             return conId.outNodeId == other.outNodeId &&
                                                    conId.outPort == other.outPort;
                                         });
                if (iter == connections.end()) break;

                shared.push_back(*iter);
                connections.erase(iter);
            }
        }

        return shared;
    };

    auto connectionsInShared  = extractSharedConnections(connectionsIn);
    auto connectionsOutShared = extractSharedConnections(connectionsOut);

    // helper function to extract and check typeIds
    auto const extractTypeIds = [&source, makeError](auto const& connections){
        QVector<QString> retval;

        for (ConnectionUuid const& conId : connections)
        {
            auto* node = source.findNodeByUuid(conId.inNodeId);
            assert(node);
            auto* port = node->port(conId.inPort);
            assert(port);

            if (!NodeDataFactory::instance().knownClass(port->typeId))
            {
                gtError() << makeError
                          << QObject::tr("Unkown node datatype '%1', id: %2, port: %3!")
                                 .arg(port->typeId,
                                      node->caption(),
                                      toString(*port));
                continue;
            }

            retval.push_back(port->typeId);
        }

        return retval;
    };

    // find datatype for input and output provider
    auto const& dtypeIn  = extractTypeIds(connectionsIn);
    auto const& dtypeOut = extractTypeIds(connectionsOut);

    if (dtypeIn.size()  != connectionsIn.size() ||
        dtypeOut.size() != connectionsOut.size()) return {};

    // setup input and output ports
    for (QString const& typeId : dtypeIn ) inputProvider->addPort(typeId);
    for (QString const& typeId : dtypeOut) outputProvider->addPort(typeId);

    // first append subgraph
    Graph* targetGraph = source.appendNode(std::move(targetGraphPtr));
    if (!targetGraph)
    {
        gtError() << makeError
                  << QObject::tr("Appending group node failed!");
        return {};
    }

    // move nodes and internal connections
    if (!::moveObjectsToGraph(source, nodes, comments, *targetGraph))
    {
        gtError() << makeError
                  << QObject::tr("Moving nodes failed!");
        return {};
    }

    // helper function to create ingoing and outgoing connections
    auto const makeConnections = [&source, targetGraph](ConnectionUuid conUuid,
                                                        auto* provider,
                                                        PortIndex index,
                                                        PortType type,
                                                        bool addToMainGraph = true,
                                                        bool addToTargetGrtaph = true) {
        if (type == PortType::Out) conUuid.reverse();

        // create connection in parent graph
        if (addToMainGraph)
        {
            ConnectionUuid newCon = conUuid;
            newCon.inNodeId = targetGraph->uuid();
            newCon.inPort   = targetGraph->portId(type, index);
            assert(newCon.isValid());

            if (type == PortType::Out) newCon.reverse();

            source.appendConnection(source.connectionId(newCon));
        }
        // create connection in subgraph
        if (addToTargetGrtaph)
        {
            conUuid.outNodeId = provider->uuid();
            conUuid.outPort   = provider->portId(invert(type), index);
            assert(conUuid .isValid());

            if (type == PortType::Out) conUuid.reverse();

            targetGraph->appendConnection(targetGraph->connectionId(conUuid));
        }
    };

    // helper function to create connections that share the same node and port
    auto const makeSharedConnetions = [makeConnections](auto& shared,
                                                        ConnectionUuid conUuid,
                                                        auto* provider,
                                                        PortIndex index,
                                                        PortType type){
        bool success = true;
        while (success)
        {
            auto iter = std::find_if(shared.begin(), shared.end(),
                                     [conUuid](ConnectionUuid const& other){
                                         return conUuid.outNodeId == other.outNodeId &&
                                                conUuid.outPort == other.outPort;
                                     });
            if ((success = (iter != shared.end())))
            {
                bool installInParent = type == PortType::Out;
                makeConnections(*iter, provider, index, type, installInParent, !installInParent);
                shared.erase(iter);
            }
        }
    };

    // make subgraph input connections
    PortIndex index{0};
    PortType type = PortType::In;
    for (ConnectionUuid const& conId : qAsConst(connectionsIn))
    {
        makeConnections(conId, inputProvider, index, type);

        makeSharedConnetions(connectionsInShared, conId, inputProvider, index, type);

        index++;
    }

    // make subgraph output connections
    index = PortIndex{0};
    type  = PortType::Out;
    for (ConnectionUuid const& conId : qAsConst(connectionsOut))
    {
        makeConnections(conId, outputProvider, index, type);

        makeSharedConnetions(connectionsOutShared, conId, outputProvider, index, type);

        index++;
    }

    return targetGraph;
}

} // namespace

Graph*
utils::groupObjects(Graph& source,
                    QString const& targetCaption,
                    View<ObjectUuid> selection)
{
    QVector<Node*> nodes;
    QVector<CommentData*> comments;

    resolveSelection(source, selection, nodes, comments);

    return ::groupObjects(source, targetCaption, nodes, comments);
}

bool
utils::expandSubgraph(std::unique_ptr<Graph> groupNode)
{
    assert(groupNode);

    auto const makeError = [logId = utils::logId(*groupNode)](gt::log::Stream& s) -> gt::log::Stream& {
        return s << logId << QObject::tr("Expanding group node failed:");
    };

    Graph* targetGraph = groupNode->parentGraph();
    if (!targetGraph)
    {
        gtError() << makeError
                  << QObject::tr("Graph has no parent graph!");
        return false;
    }

    // create undo command
    auto modifyCmd = targetGraph->modify();
    Q_UNUSED(modifyCmd);

    auto const& conModel = targetGraph->connectionModel();

    auto* inputProvider  = groupNode->inputProvider();
    auto* outputProvider = groupNode->outputProvider();
    assert(inputProvider);
    assert(outputProvider);

    // gather input and output connections
    QVector<ConnectionUuid> expandedInputConnections, expandedOutputConnections;

    // extra scope since group node will be deleted eventually -> avoid dangling
    // references
    {
        // "flatten" connections between parent graph and subgraph
        auto convertConnection = [targetGraph, &conModel, &groupNode]
                                 (ConnectionId conId,
                                  auto& convertedConnections,
                                  PortType type){
            ConnectionUuid conUuid = groupNode->connectionUuid(conId);

            bool const isInput = type == PortType::In;
            if (isInput) conUuid.reverse();

            auto connections = conModel.iterate(groupNode->id(), conUuid.outPort);
            for (ConnectionDetail<NodeId> connection : connections)
            {
                NodeId targetNodeId = connection.node;
                Node* targetNode = targetGraph->findNode(targetNodeId);
                assert(targetNode);
                conUuid.outNodeId = targetNode->uuid();
                conUuid.outPort   = connection.port;

                convertedConnections.push_back(isInput ? conUuid.reversed() : conUuid);
            }
        };

        auto const& groupConModel = groupNode->connectionModel();

        PortType type = PortType::Out;
        auto inputCons  = groupConModel.iterateConnections(inputProvider->id(), type);
        for (ConnectionId conId : inputCons)
        {
            convertConnection(conId, expandedInputConnections, type);
        }

        type = PortType::In;
        auto outputCons = groupConModel.iterateConnections(outputProvider->id(), type);
        for (ConnectionId conId : outputCons)
        {
            convertConnection(conId, expandedInputConnections, type);
        }
    }

    // delete provider nodes
    if (!groupNode->deleteNode(groupNode->inputNode()->id()) ||
        !groupNode->deleteNode(groupNode->outputNode()->id()))
    {
        gtError() << makeError
                  << QObject::tr("Failed to remove provider nodes!");
        return false;
    }

    inputProvider = nullptr;
    outputProvider = nullptr;
    auto nodes = groupNode->nodes();

    // update node positions
    QPolygonF selectionPoly;
    std::transform(nodes.begin(), nodes.end(),
                   std::back_inserter(selectionPoly), [](auto const* node){
        return node->pos();
    });

    auto boundingRect = selectionPoly.boundingRect();
    auto center = boundingRect.center();
    for (Node* node : qAsConst(nodes))
    {
        auto offset = (node->pos() - center);
        node->setPos(groupNode->pos() + offset);
    }

    // move objects
    if (!moveObjectsToGraph(*groupNode, *targetGraph))
    {
        gtError() << makeError
                  << QObject::tr("Failed to move internal nodes)");
        return false;
    }

    // delete group node
    groupNode.reset();

    // install connections to moved nodes
    for (auto* connections : {&expandedInputConnections, &expandedOutputConnections})
    {
        for (ConnectionUuid const& conUuid : qAsConst(*connections))
        {
            targetGraph->appendConnection(targetGraph->connectionId(conUuid));
        }
    }

    return true;
}

Graph*
utils::duplicateGraph(Graph& source)
{
    GtObject* parent = source.parentObject();

    std::unique_ptr<Graph> newGraph = makeCopy(source);

    newGraph->setPos(newGraph->pos() + Position{50, 50});

    if (auto* parentGraph = source.parentGraph())
    {
        return parentGraph->appendNode<Graph>(std::move(newGraph));
    }

    if (!parent->appendChild(newGraph.get()))
    {
        return {};
    }

    newGraph->updateObjectName();
    return newGraph.release();
}
