/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "test_helper.h"

#include "node/test_dynamic.h"

#include <gt_objectmemento.h>
#include <gt_objectmementodiff.h>
#include <gt_objectfactory.h>

using namespace intelli;
TEST(Graph, ancestors_and_descendants)
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
}

// the connections are inside an object group, thus chaning the connections
// only requires special care when reverting a diff
TEST(Graph, restore_connections_only_on_memento_diff)
{
    using namespace intelli;

    Graph graph;
    graph.setFactory(gtObjectFactory);
    
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
