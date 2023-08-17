/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "test_helper.h"

#include <gt_objectmemento.h>
#include <gt_objectmementodiff.h>
#include <gt_objectfactory.h>

using namespace intelli;
TEST(Graph, basic_graph)
{
    Graph graph;

    ASSERT_TRUE(test::buildTestGraph(graph));

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

    dag::debugGraph(graph.dag());

    // delete node C

    ASSERT_TRUE(graph.deleteNode(C_id));

    EXPECT_EQ(graph.connections().size(), 2);
    EXPECT_EQ(graph.nodes().size(), 4);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(B_id), B);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(graph.findNode(C_id), nullptr);

    dag::debugGraph(graph.dag());

    // delete node B

    ASSERT_TRUE(graph.deleteNode(B_id));

    EXPECT_EQ(graph.connections().size(), 0);
    EXPECT_EQ(graph.nodes().size(), 3);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(graph.findNode(B_id), nullptr);
    EXPECT_EQ(graph.findNode(C_id), nullptr);

    dag::debugGraph(graph.dag());

    // delete all

    graph.clear();

    EXPECT_EQ(graph.connections().size(), 0);
    EXPECT_EQ(graph.nodes().size(), 0);

    dag::debugGraph(graph.dag());

    // check deleting A does not work

    // A cannot be found
    ASSERT_FALSE(graph.deleteNode(A_id));
}

// when reverting a diff the DAG must be updated accordingly
TEST(Graph, restore_nodes_and_connections_on_memento_diff)
{
    using namespace intelli;

    Graph graph;
    graph.setFactory(gtObjectFactory);

    ASSERT_TRUE(test::buildTestGraph(graph));

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

        EXPECT_TRUE(consIn.contains(ConnectionId{A_id, PortIndex(0), C_id, PortIndex(0)}));
        EXPECT_TRUE(consIn.contains(ConnectionId{B_id, PortIndex(0), C_id, PortIndex(1)}));

        QVector<ConnectionId> const& consOut = graph.findConnections(C_id, PortType::Out); // C
        ASSERT_EQ(consOut.size(), 1);

        EXPECT_TRUE(consOut.contains(ConnectionId{C_id, PortIndex(0), D_id, PortIndex(0)}));

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

    dag::debugGraph(graph.dag());

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

    dag::debugGraph(graph.dag());

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

    ASSERT_TRUE(test::buildTestGraph(graph));

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

    dag::debugGraph(graph.dag());

    GtObjectMemento mementoBefore = graph.toMemento();

    // delete node c

    ConnectionId connectionToDelete{ C_id, PortIndex(0), D_id, PortIndex(0) };
    ASSERT_TRUE(graph.deleteConnection(connectionToDelete));

    EXPECT_EQ(graph.connections().size(), 4);
    EXPECT_EQ(graph.nodes().size(), 5);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(B_id), B);
    EXPECT_EQ(graph.findNode(C_id), C);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(graph.findConnections(C_id, PortType::Out).size(), 0);

    dag::debugGraph(graph.dag());

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

    dag::debugGraph(graph.dag());

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
