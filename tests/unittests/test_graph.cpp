/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "test_helper.h"

#include "node/test_dynamic.h"

#include "intelli/connection.h"

#include <gt_objectmemento.h>
#include <gt_objectmementodiff.h>
#include <gt_objectfactory.h>

using namespace intelli;

TEST(Graph, root_graph)
{
    auto rootGraphPtr = std::make_unique<Graph>();
    Graph* rootGraph = rootGraphPtr.get();
    EXPECT_EQ(rootGraph->rootGraph(), rootGraph);

    auto subgraphPtr = std::make_unique<Graph>();
    Graph* subgraph = subgraphPtr.get();
    rootGraph->appendNode(std::move(subgraphPtr));

    EXPECT_EQ(rootGraph->rootGraph(), rootGraph);
    EXPECT_EQ(subgraph->rootGraph(), rootGraph);

    GtObject root;
    ASSERT_TRUE(root.appendChild(rootGraph));

    rootGraphPtr.release(); // root now owns rootGraph
    EXPECT_EQ(rootGraph->rootGraph(), rootGraph);
    EXPECT_EQ(subgraph->rootGraph(), rootGraph);
}

/// input and output provider generate "virtual" (i.e. hidden) ports to simplify
/// connecting to parent graph
TEST(Graph, input_and_output_provider)
{
    Graph graph;

    GraphBuilder builder(graph);
    auto sub = builder.addGraph({}, {});
    PortId inPort1  = sub.inNode.addOutPort(typeId<DoubleData>());
    PortId inPort2  = sub.inNode.addOutPort(typeId<DoubleData>());
    EXPECT_TRUE(inPort1.isValid());
    EXPECT_TRUE(inPort2.isValid());

    PortId outPort1 = sub.outNode.addInPort(typeId<DoubleData>());
    EXPECT_TRUE(outPort1.isValid());

    EXPECT_EQ(sub.inNode.ports( PortType::In).size(),
              sub.inNode.ports( PortType::Out).size());
    EXPECT_EQ(sub.outNode.ports( PortType::In).size(),
              sub.outNode.ports( PortType::Out).size());

    EXPECT_EQ(sub.graph.ports(PortType::In).size(),
              (sub.inNode.ports(PortType::In).size() +
               sub.outNode.ports(PortType::Out).size()));
}

TEST(Graph, connection_model_iterate_over_connections)
{
    Graph graph;

    test::buildLinearGraph(graph);

    auto& conModel = graph.connectionModel();
    auto conData = conModel.find(C_id);
    ASSERT_TRUE(conData != conModel.end());

    auto iIn  = conData->iterateConnections(PortType::In);
    auto iOut = conData->iterateConnections(PortType::Out);
    auto iAll = conData->iterateConnections();
    EXPECT_EQ(std::distance(iIn.begin(), iIn.end()), 2);
    EXPECT_EQ(std::distance(iOut.begin(), iOut.end()), 1);
    EXPECT_EQ(std::distance(iAll.begin(), iAll.end()), 3);

    auto riIn  = iIn.reverse();
    auto riOut = iOut.reverse();
    auto riAll = iAll.reverse();
    EXPECT_EQ(std::distance(riIn.begin(), riIn.end()), 2);
    EXPECT_EQ(std::distance(riOut.begin(), riOut.end()), 1);
    EXPECT_EQ(std::distance(riAll.begin(), riAll.end()), 3);

    auto iter = iAll.begin();
    auto endIter = iAll.end();
    decltype(endIter) nullIter{};
    EXPECT_TRUE(endIter == nullIter);

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, graph.connectionId(B_id, PortIndex(0), C_id, PortIndex(0)));

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, graph.connectionId(B_id, PortIndex(0), C_id, PortIndex(1)));

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, graph.connectionId(C_id, PortIndex(0), D_id, PortIndex(0)));

    EXPECT_TRUE(iter == endIter);
    EXPECT_TRUE(iter == nullIter);

    // check forwarding methods of connection model
    auto fIn = conModel.iterateConnections(C_id, PortType::In);
    EXPECT_TRUE(std::equal(fIn.begin(), fIn.end(), iIn.begin()));
    auto fOut = conModel.iterateConnections(C_id, PortType::Out);
    EXPECT_TRUE(std::equal(fOut.begin(), fOut.end(), iOut.begin()));
    auto fAll = conModel.iterateConnections(C_id);
    EXPECT_TRUE(std::equal(fAll.begin(), fAll.end(), iAll.begin()));
}

TEST(Graph, connection_model_iterate_over_connections_by_port)
{
    Graph graph;

    test::buildLinearGraph(graph);

    auto& conModel = graph.connectionModel();
    auto conData = conModel.find(B_id);
    ASSERT_TRUE(conData != conModel.end());

    auto iPort = conData->iterateConnections(PortId(2));
    auto iter = iPort.begin();
    auto endIter = iPort.end();
    decltype(endIter) nullIter{};
    EXPECT_TRUE(endIter == nullIter);

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, graph.connectionId(B_id, PortIndex(0), C_id, PortIndex(0)));

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, graph.connectionId(B_id, PortIndex(0), C_id, PortIndex(1)));

    EXPECT_TRUE(iter == endIter);
    EXPECT_TRUE(iter == nullIter);

    EXPECT_EQ(std::distance(iPort.begin(), iPort.end()), 2);

    auto riPort = iPort.reverse();
    EXPECT_EQ(std::distance(riPort.begin(), riPort.end()), 2);
}

TEST(Graph, connection_model_iterate_over_connected_nodes)
{
    Graph graph;

    test::buildLinearGraph(graph);

    auto& conModel = graph.connectionModel();
    auto conData = conModel.find(B_id);
    ASSERT_TRUE(conData != conModel.end());

    auto iIn  = conData->iterateNodes(PortType::In);
    auto iOut = conData->iterateNodes(PortType::Out);
    auto iAll = conData->iterateNodes();
    EXPECT_EQ(std::distance(iIn.begin(), iIn.end()), 1);
    EXPECT_EQ(std::distance(iOut.begin(), iOut.end()), 2);
    EXPECT_EQ(std::distance(iAll.begin(), iAll.end()), 3);

    auto riIn  = iIn.reverse();
    auto riOut = iOut.reverse();
    auto riAll = iAll.reverse();
    EXPECT_EQ(std::distance(riIn.begin(), riIn.end()), 1);
    EXPECT_EQ(std::distance(riOut.begin(), riOut.end()), 2);
    EXPECT_EQ(std::distance(riAll.begin(), riAll.end()), 3);

    auto iter = iAll.begin();
    auto endIter = iAll.end();
    decltype(endIter) nullIter{};
    EXPECT_TRUE(endIter == nullIter);

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, A_id);

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, C_id);

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, C_id);

    EXPECT_TRUE(iter == endIter);
    EXPECT_TRUE(iter == nullIter);
}

TEST(Graph, connection_model_iterate_over_connected_nodes_by_port)
{
    Graph graph;

    test::buildLinearGraph(graph);

    auto& conModel = graph.connectionModel();
    auto conData = conModel.find(B_id);
    ASSERT_TRUE(conData != conModel.end());

    auto iPort = conData->iterateNodes(PortId(2));
    auto iter = iPort.begin();
    auto endIter = iPort.end();
    decltype(endIter) nullIter{};
    EXPECT_TRUE(endIter == nullIter);

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, C_id);

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, C_id);

    EXPECT_TRUE(iter == endIter);
    EXPECT_TRUE(iter == nullIter);

    EXPECT_EQ(std::distance(iPort.begin(), iPort.end()), 2);

    auto riPort = iPort.reverse();
    EXPECT_EQ(std::distance(riPort.begin(), riPort.end()), 2);
}

struct IntProxy
{
    using value_type = int;
    using reference = value_type&;
    using pointer = value_type*;
    template <typename Iter> void init(Iter&) {};
    template <typename Iter> reference get(Iter& i) { return (*i).value; };
    template <typename Iter> void advance(Iter& i) { ++i; };
};

TEST(Graph, connection_model_custom_iterator)
{
    struct MyStruct
    {
        int value;
        QString str;
    };
    QVector<MyStruct> data{
        {42, "Test"},
        {10, "32"}
    };

    // use a proxy to access `value` member of `MyStruct`
    auto iter = makeProxy<IntProxy>(data.begin(), data.end());

    QVector<int> reference{42, 10};
    ASSERT_EQ(iter.size(), reference.size());
    EXPECT_TRUE(std::equal(iter.begin(), iter.end(),
                           reference.begin(), reference.end()));
}

TEST(Graph, predessecors_and_successors)
{
    Graph graph;

    ASSERT_TRUE(test::buildBasicGraph(graph));

    EXPECT_EQ(graph.connections().size(), 5);
    EXPECT_EQ(graph.nodes().size(), 5);

    auto* A = graph.findNode(A_id);
    auto* B = graph.findNode(B_id);
    auto* C = graph.findNode(C_id);
    auto* D = graph.findNode(D_id);
    auto* E = graph.findNode(E_id);

    ASSERT_NE(A, nullptr);
    ASSERT_NE(B, nullptr);
    ASSERT_NE(C, nullptr);
    ASSERT_NE(D, nullptr);
    ASSERT_NE(E, nullptr);

    debug(graph);

    EXPECT_EQ(graph.findDependencies(A->id()).size(), 0);
    EXPECT_EQ(graph.findDependentNodes(A->id()), (QVector<NodeId>{C->id(), D->id()}));

    EXPECT_EQ(graph.findDependencies(B->id()).size(), 0);
    EXPECT_EQ(graph.findDependentNodes(B->id()), (QVector<NodeId>{C->id(), D->id(), E->id()}));

    EXPECT_EQ(graph.findDependencies(C->id()), (QVector<NodeId>{A->id(), B->id()}));
    EXPECT_EQ(graph.findDependentNodes(C->id()), (QVector<NodeId>{D->id()}));

    EXPECT_EQ(graph.findDependencies(D->id()), (QVector<NodeId>{C->id(), A->id(), B->id()}));
    EXPECT_EQ(graph.findDependentNodes(D->id()).size(), 0);

    EXPECT_EQ(graph.findDependencies(E->id()), (QVector<NodeId>{B->id()}));
    EXPECT_EQ(graph.findDependentNodes(E->id()).size(), 0);
}

/// Check successor and predecessor nodes of group input and output nodes in
/// a graph where input and output nodes are connected to each other
TEST(Graph, predessecors_and_successors_in_graph_with_forwarding_group)
{
    Graph graph;

    ASSERT_TRUE(test::buildGraphWithForwardingGroup(graph));

    auto* A = graph.findNode(A_id);
    auto* B = graph.findNode(B_id);
    auto* C = graph.findNode(C_id);
    auto* D = graph.findNode(D_id);
    auto* E = graph.findNode(E_id);
    auto* IN  = graph.findNodeByUuid(group_input_uuid);
    auto* OUT = graph.findNodeByUuid(group_output_uuid);

    ASSERT_NE(A, nullptr);
    ASSERT_NE(B, nullptr);
    ASSERT_NE(C, nullptr);
    ASSERT_NE(D, nullptr);
    ASSERT_NE(E, nullptr);

    ASSERT_NE(IN,  nullptr);
    ASSERT_NE(OUT, nullptr);

    debug(graph);

    auto& conModel = graph.globalConnectionModel();

    auto inNodeData = conModel.find(IN->uuid());
    ASSERT_NE(inNodeData, conModel.end());

    EXPECT_EQ(inNodeData->successors.size(), 2);
    EXPECT_EQ(inNodeData->successors.at(0).node, OUT->uuid());
    EXPECT_EQ(inNodeData->successors.at(0).port, OUT->portId(PortType::In, PortIndex(0)));
    EXPECT_EQ(inNodeData->successors.at(1).node, OUT->uuid());
    EXPECT_EQ(inNodeData->successors.at(1).port, OUT->portId(PortType::In, PortIndex(1)));

    EXPECT_EQ(inNodeData->predecessors.size(), 2);
    EXPECT_EQ(inNodeData->predecessors.at(0).node, A->uuid());
    EXPECT_EQ(inNodeData->predecessors.at(0).port, A->portId(PortType::Out, PortIndex(0)));
    EXPECT_EQ(inNodeData->predecessors.at(1).node, B->uuid());
    EXPECT_EQ(inNodeData->predecessors.at(1).port, B->portId(PortType::Out, PortIndex(0)));

    auto outNodeData = conModel.find(OUT->uuid());
    ASSERT_NE(outNodeData, conModel.end());

    EXPECT_EQ(outNodeData->successors.size(), 2);
    EXPECT_EQ(outNodeData->successors.at(0).node, C->uuid());
    EXPECT_EQ(outNodeData->successors.at(0).port, outNodeData->successors.at(0).sourcePort);
    EXPECT_EQ(outNodeData->successors.at(1).node, C->uuid());
    EXPECT_EQ(outNodeData->successors.at(1).port, outNodeData->successors.at(1).sourcePort);

    EXPECT_EQ(outNodeData->predecessors.size(), 2);
    EXPECT_EQ(outNodeData->predecessors.at(0).node, IN->uuid());
    EXPECT_EQ(outNodeData->predecessors.at(0).port, IN->portId(PortType::Out, PortIndex(0)));
    EXPECT_EQ(outNodeData->predecessors.at(1).node, IN->uuid());
    EXPECT_EQ(outNodeData->predecessors.at(1).port, IN->portId(PortType::Out, PortIndex(1)));
}

TEST(Graph, remove_connections_on_node_deletion)
{
    Graph graph;
    
    ASSERT_TRUE(test::buildBasicGraph(graph));

    EXPECT_EQ(graph.connections().size(), 5);
    EXPECT_EQ(graph.nodes().size(), 5);

    auto* A = graph.findNode(A_id);
    auto* B = graph.findNode(B_id);
    auto* C = graph.findNode(C_id);
    auto* D = graph.findNode(D_id);
    auto* E = graph.findNode(E_id);

    EXPECT_NE(A, nullptr);
    EXPECT_NE(B, nullptr);
    EXPECT_NE(C, nullptr);
    EXPECT_NE(D, nullptr);
    EXPECT_NE(E, nullptr);

    EXPECT_EQ(graph.findNode(NodeId(5)), nullptr); // no extra node can be found
    
    debug(graph);

    // delete node C

    ASSERT_TRUE(graph.deleteNode(C_id));

    EXPECT_EQ(graph.connections().size(), 2);
    EXPECT_EQ(graph.nodes().size(), 4);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(B_id), B);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(graph.findNode(C_id), nullptr);
    
    debug(graph);

    // delete node B

    ASSERT_TRUE(graph.deleteNode(B_id));

    EXPECT_EQ(graph.connections().size(), 0);
    EXPECT_EQ(graph.nodes().size(), 3);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(graph.findNode(B_id), nullptr);
    EXPECT_EQ(graph.findNode(C_id), nullptr);
    
    debug(graph);

    // delete all
    
    graph.clearGraph();

    EXPECT_EQ(graph.connections().size(), 0);
    EXPECT_EQ(graph.nodes().size(), 0);
    
    debug(graph);

    // check deleting A does not work

    // A cannot be found
    ASSERT_FALSE(graph.deleteNode(A_id));
}

// when removing a port connections should be removed
TEST(Graph, remove_connections_on_port_deletion)
{
    using namespace intelli;

    Graph graph;

    GraphBuilder builder(graph);

    TestDynamicNode* dynamicNode = nullptr;

    ConnectionId conId1 = invalid<ConnectionId>();
    ConnectionId conId2 = invalid<ConnectionId>();
    ConnectionId conId3 = invalid<ConnectionId>();

    try
    {
        auto& A = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("A"));
        auto& B = builder.addNode<TestDynamicNode>();
        B.setCaption(QStringLiteral("B"));
        dynamicNode = &B;

        setNodeProperty(A, QStringLiteral("value"), 42);

        ASSERT_EQ(B.addInPort(typeId<DoubleData>()), PortId(0));
        ASSERT_EQ(B.addInPort(typeId<DoubleData>()), PortId(1));
        ASSERT_EQ(B.addInPort(typeId<DoubleData>()), PortId(2));

        ASSERT_TRUE(A.port(PortId(0))); // A should have only one output port

        ASSERT_EQ(A.id(), A_id);
        ASSERT_EQ(B.id(), B_id);

        conId1 = builder.connect(A, PortIndex(0), B, PortIndex(0));
        conId2 = builder.connect(A, PortIndex(0), B, PortIndex(1));
        conId3 = builder.connect(A, PortIndex(0), B, PortIndex(2));
    }
    catch (std::logic_error const& e)
    {
        gtError() << e.what();
        ASSERT_NO_THROW(throw e);
    }

    ASSERT_TRUE(dynamicNode);
    ASSERT_TRUE(dynamicNode->ports(PortType::In).size() == 3);

    EXPECT_EQ(graph.nodes().size(), 2);
    EXPECT_EQ(graph.connections().size(), 3);
    
    debug(graph);

    // delete 1. connection
    EXPECT_TRUE(graph.deleteConnection(conId1));

    EXPECT_FALSE(graph.findConnection(conId1));
    EXPECT_TRUE(graph.findConnection(conId2));
    EXPECT_TRUE(graph.findConnection(conId3));
    
    debug(graph);

    // no connections removed when deleting unconnected port no. 1
    ASSERT_TRUE(dynamicNode->removePort(PortId(0)));
    ASSERT_TRUE(dynamicNode->ports(PortType::In).size() == 2);

    EXPECT_FALSE(graph.findConnection(conId1));
    EXPECT_TRUE(graph.findConnection(conId2));
    EXPECT_TRUE(graph.findConnection(conId3));
    
    debug(graph);

    // connections are removed when deleting port no. 2
    ASSERT_TRUE(dynamicNode->removePort(PortId(1)));
    ASSERT_TRUE(dynamicNode->ports(PortType::In).size() == 1);

    EXPECT_FALSE(graph.findConnection(conId1));
    EXPECT_FALSE(graph.findConnection(conId2));
    EXPECT_TRUE(graph.findConnection(conId3));
    
    debug(graph);

    // connections are removed when deleting port no. 2
    ASSERT_TRUE(dynamicNode->removePort(PortId(2)));
    ASSERT_TRUE(dynamicNode->ports(PortType::In).size() == 0);

    EXPECT_FALSE(graph.findConnection(conId1));
    EXPECT_FALSE(graph.findConnection(conId2));
    EXPECT_FALSE(graph.findConnection(conId3));

    EXPECT_EQ(graph.nodes().size(), 2);
    EXPECT_EQ(graph.connections().size(), 0);
    
    debug(graph);
}

// when reverting a diff the DAG must be updated accordingly
TEST(Graph, restore_nodes_and_connections_on_memento_diff)
{
    using namespace intelli;

    Graph graph;
    graph.setFactory(gtObjectFactory);
    
    ASSERT_TRUE(test::buildGraphWithGroup(graph));

    EXPECT_EQ(graph.connections().size(), 5);
    EXPECT_EQ(graph.nodes().size(), 5);

    auto* A = graph.findNode(A_id);
    auto* B = graph.findNode(B_id);
    auto* C = graph.findNode(C_id);
    auto* D = graph.findNode(D_id);
    auto* E = graph.findNode(E_id);

    EXPECT_NE(A, nullptr);
    EXPECT_NE(B, nullptr);
    EXPECT_NE(C, nullptr);
    EXPECT_NE(D, nullptr);
    EXPECT_NE(E, nullptr);

    debug(graph);

    auto const checkConnectionsOfNodeC = [&graph](){
        gtDebug() << "checking connections of node C...";

        QVector<ConnectionId> const& consIn = graph.findConnections(C_id, PortType::In); // C
        ASSERT_EQ(consIn.size(), 2);

        EXPECT_TRUE(consIn.contains(graph.connectionId(A_id, PortIndex(0), C_id, PortIndex(0))));
        EXPECT_TRUE(consIn.contains(graph.connectionId(B_id, PortIndex(0), C_id, PortIndex(1))));

        QVector<ConnectionId> const& consOut = graph.findConnections(C_id, PortType::Out); // C
        ASSERT_EQ(consOut.size(), 1);

        EXPECT_TRUE(consOut.contains(graph.connectionId(C_id, PortIndex(0), D_id, PortIndex(0))));

        EXPECT_EQ(graph.findConnections(C_id), consIn + consOut);
    };

    checkConnectionsOfNodeC();

    GtObjectMemento mementoBefore = graph.toMemento();

    // delete node c

    gtDebug() << "Deleting node C...";
    ASSERT_TRUE(graph.deleteNode(C_id));

    EXPECT_EQ(graph.connections().size(), 2);
    EXPECT_EQ(graph.nodes().size(), 4);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(B_id), B);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(graph.findNode(C_id), nullptr);

    // Node C does no longer exists -> its connections have been deleted as well
    EXPECT_EQ(graph.findConnections(C_id).size(), 0);
    
    debug(graph);

    GtObjectMemento mementoAfter = graph.toMemento();

    // revert memento diff

    gtDebug() << "Reverting diff...";
    GtObjectMementoDiff diff(mementoBefore, mementoAfter);

    ASSERT_TRUE(graph.revertDiff(diff));

    // graph should have restored fully
    EXPECT_EQ(graph.connections().size(), 5);
    EXPECT_EQ(graph.nodes().size(), 5);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(B_id), B);
    EXPECT_NE(graph.findNode(C_id), nullptr);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    checkConnectionsOfNodeC();
    
    debug(graph);

    // apply memento diff

    gtDebug() << "Applying diff...";
    ASSERT_TRUE(graph.applyDiff(diff));

    EXPECT_EQ(graph.connections().size(), 2);
    EXPECT_EQ(graph.nodes().size(), 4);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(B_id), B);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(graph.findNode(C_id), nullptr);

    // Node C does no longer exists -> its connections have been deleted as well
    EXPECT_EQ(graph.findConnections(C_id).size(), 0);

    debug(graph);
}

// the connections are inside an object group, thus chaning the connections
// only requires special care when reverting a diff
TEST(Graph, restore_connections_only_on_memento_diff)
{
    using namespace intelli;

    Graph graph;
    graph.setFactory(gtObjectFactory);
    
    ASSERT_TRUE(test::buildGraphWithGroup(graph));

    EXPECT_EQ(graph.connections().size(), 5);
    EXPECT_EQ(graph.nodes().size(), 5);

    auto* A = graph.findNode(A_id);
    auto* B = graph.findNode(B_id);
    auto* C = graph.findNode(C_id);
    auto* D = graph.findNode(D_id);
    auto* E = graph.findNode(E_id);

    EXPECT_NE(A, nullptr);
    EXPECT_NE(B, nullptr);
    EXPECT_NE(C, nullptr);
    EXPECT_NE(D, nullptr);
    EXPECT_NE(E, nullptr);

    EXPECT_EQ(graph.findConnections(C_id, PortType::Out).size(), 1);
    
    debug(graph);

    GtObjectMemento mementoBefore = graph.toMemento();

    // delete node c

    ConnectionId connectionToDelete = graph.connectionId(C_id, PortIndex(0), D_id, PortIndex(0));
    ASSERT_TRUE(graph.deleteConnection(connectionToDelete));

    EXPECT_EQ(graph.connections().size(), 4);
    EXPECT_EQ(graph.nodes().size(), 5);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(B_id), B);
    EXPECT_EQ(graph.findNode(C_id), C);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(graph.findConnections(C_id, PortType::Out).size(), 0);
    
    debug(graph);

    GtObjectMemento mementoAfter = graph.toMemento();

    // revert memento diff

    GtObjectMementoDiff diff(mementoBefore, mementoAfter);

    graph.revertDiff(diff);

    EXPECT_EQ(graph.connections().size(), 5);
    EXPECT_EQ(graph.nodes().size(), 5);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(B_id), B);
    EXPECT_EQ(graph.findNode(C_id), C);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(graph.findConnections(C_id, PortType::Out).size(), 1);
    EXPECT_TRUE(graph.findConnections(C_id, PortType::Out).contains(connectionToDelete));
    
    debug(graph);

    // apply memento diff

    graph.applyDiff(diff);

    EXPECT_EQ(graph.connections().size(), 4);
    EXPECT_EQ(graph.nodes().size(), 5);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(B_id), B);
    EXPECT_EQ(graph.findNode(C_id), C);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(graph.findConnections(C_id, PortType::Out).size(), 0);
}

TEST(Graph, move_node_to_subgraph)
{
    Graph root;

    ASSERT_TRUE(test::buildGraphWithGroup(root));

    auto const& subgraphs = root.graphNodes();
    ASSERT_FALSE(subgraphs.empty());

    Graph* subgraph = subgraphs[0];
    ASSERT_TRUE(subgraph);
    Node* nodeA = root.findNode(A_id);
    ASSERT_TRUE(nodeA);

    ASSERT_EQ(nodeA->parent(), &root);

    // before move
    EXPECT_TRUE(root.findNode(A_id));
    EXPECT_TRUE(root.findNodeByUuid(A_uuid));
    EXPECT_NE(subgraph->findNode(A_id), nodeA);
    EXPECT_FALSE(subgraph->findNodeByUuid(A_uuid));

    // move node
    EXPECT_TRUE(root.moveNode(A_id, *subgraph));
    EXPECT_EQ(nodeA->parent(), subgraph);

    // after move
    EXPECT_FALSE(root.findNode(A_id));
    EXPECT_TRUE(root.findNodeByUuid(A_uuid));
    EXPECT_TRUE(subgraph->findNode(A_id));
    EXPECT_TRUE(subgraph->findNodeByUuid(A_uuid));
}

TEST(Graph, move_node_to_other_graph)
{
    Graph graph1;
    Graph graph2;

    ASSERT_TRUE(test::buildLinearGraph(graph1));
    ASSERT_TRUE(test::buildLinearGraph(graph2));

    Node* nodeA = graph1.findNode(A_id);
    ASSERT_TRUE(nodeA);

    ASSERT_EQ(nodeA->parent(), &graph1);

    // before move
    EXPECT_TRUE(graph1.findNode(A_id));
    EXPECT_TRUE(graph1.findNodeByUuid(A_uuid));
    EXPECT_FALSE(graph2.findNode(A_id));
    EXPECT_FALSE(graph2.findNodeByUuid(A_uuid));

    // move node
    EXPECT_TRUE(graph1.moveNode(A_id, graph2));
    EXPECT_EQ(nodeA->parent(), &graph2);

    // after move
    EXPECT_FALSE(graph1.findNode(A_id));
    EXPECT_FALSE(graph1.findNodeByUuid(A_uuid));
    EXPECT_TRUE(graph2.findNode(A_id));
    EXPECT_TRUE(graph2.findNodeByUuid(A_uuid));
}

TEST(Graph, move_nodes_to_other_graph)
{
    Graph graph1;
    Graph graph2;

    ASSERT_TRUE(test::buildLinearGraph(graph1));

    auto model = graph1.connectionModel();
    int connections = model.size();

    // before move
    EXPECT_FALSE(graph1.connectionModel().empty());
    EXPECT_TRUE(graph2.connectionModel().empty());
    EXPECT_TRUE(graph1.connectionModel() == model);
    EXPECT_FALSE(graph2.connectionModel() == model);
    EXPECT_EQ(graph1.connectionModel().size(), connections);
    EXPECT_NE(graph2.connectionModel().size(), connections);

    // move node
    EXPECT_TRUE(graph1.moveNodesAndConnections({A_id, B_id, C_id, D_id}, graph2));

    // after move
    EXPECT_TRUE(graph1.connectionModel().empty());
    EXPECT_FALSE(graph2.connectionModel().empty());
    EXPECT_FALSE(graph1.connectionModel() == model);
    EXPECT_TRUE(graph2.connectionModel() == model);
    EXPECT_NE(graph1.connectionModel().size(), connections);
    EXPECT_EQ(graph2.connectionModel().size(), connections);
}
