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
#include "intelli/data/file.h"
#include "intelli/data/int.h"
#include "intelli/data/double.h"
#include "intelli/data/string.h"
#include "intelli/data/bool.h"

#include "intelli/node/graphinputprovider.h"
#include "intelli/node/graphoutputprovider.h"

#include "intelli/connection.h"
#include "intelli/utilities.h"

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

/// Clearing a graph causes all nodes and connections to be removed except
/// providers. Added ports are removed as well.
TEST(Graph, input_and_output_provider_clear_graph)
{
    Graph graph;

    EXPECT_EQ(graph.nodes().size(), 2);
    EXPECT_EQ(graph.connections().size(), 0);
    ASSERT_TRUE(graph.inputProvider());
    ASSERT_TRUE(graph.outputProvider());

    GraphBuilder builder{graph};
    Node& a = builder.addNode("intelli::NumberMathNode");
    Node& b = builder.addNode("intelli::NumberMathNode");

    graph.addInPort(intelli::typeId<IntData>());
    graph.addInPort(intelli::typeId<DoubleData>());
    graph.addOutPort(intelli::typeId<IntData>());
    graph.addOutPort(intelli::typeId<DoubleData>());

    ASSERT_EQ(graph.ports(PortType::In).size(), 2);
    ASSERT_EQ(graph.ports(PortType::Out).size(), 2);

    builder.connect(*graph.inputProvider(), PortIndex(0), a, PortIndex(0));
    builder.connect(*graph.inputProvider(), PortIndex(1), a, PortIndex(1));
    builder.connect(a, PortIndex(0), b, PortIndex(0));
    builder.connect(a, PortIndex(0), b, PortIndex(1));
    builder.connect(a, PortIndex(0), *graph.outputProvider(), PortIndex(0));
    builder.connect(b, PortIndex(0), *graph.outputProvider(), PortIndex(1));

    EXPECT_EQ(graph.nodes().size(), 4);
    EXPECT_EQ(graph.connections().size(), 6);

    graph.clearGraph();

    EXPECT_EQ(graph.nodes().size(), 2);
    EXPECT_EQ(graph.connections().size(), 0);
    ASSERT_TRUE(graph.inputProvider());
    ASSERT_TRUE(graph.outputProvider());

    ASSERT_EQ(graph.ports(PortType::In).size(), 0);
    ASSERT_EQ(graph.ports(PortType::Out).size(), 0);
}

/// providers are hidden unless explicitly initialized. Subgraphs always
/// initialize their providers
TEST(Graph, input_and_output_provider_initialize)
{
    Graph graph;

    ASSERT_TRUE(graph.inputProvider());
    EXPECT_TRUE(graph.inputProvider()->isUserHidden());
    ASSERT_TRUE(graph.outputProvider());
    EXPECT_TRUE(graph.outputProvider()->isUserHidden());

    auto subgraphPtr = std::make_unique<Graph>();
    Graph* subgraph =subgraphPtr.get();

    ASSERT_TRUE(subgraph->inputProvider());
    ASSERT_TRUE(subgraph->inputProvider()->isUserHidden());
    ASSERT_TRUE(subgraph->outputProvider());
    ASSERT_TRUE(subgraph->outputProvider()->isUserHidden());

    // adding subgraph will initialize providers
    subgraph = graph.appendNode(std::move(subgraphPtr));
    ASSERT_TRUE(subgraph);

    ASSERT_TRUE(subgraph->inputProvider());
    EXPECT_FALSE(subgraph->inputProvider()->isUserHidden());
    ASSERT_TRUE(subgraph->outputProvider());
    EXPECT_FALSE(subgraph->outputProvider()->isUserHidden());

    ASSERT_TRUE(graph.inputProvider());
    EXPECT_TRUE(graph.inputProvider()->isUserHidden());
    ASSERT_TRUE(graph.outputProvider());
    EXPECT_TRUE(graph.outputProvider()->isUserHidden());

    graph.initInputOutputProviders();

    ASSERT_TRUE(graph.inputProvider());
    EXPECT_FALSE(graph.inputProvider()->isUserHidden());
    ASSERT_TRUE(graph.outputProvider());
    EXPECT_FALSE(graph.outputProvider()->isUserHidden());
}

/// adding ports to graph will add ports to input and output provider respectively
TEST(Graph, input_and_output_provider_add_ports)
{
    Graph graph;

    GraphBuilder builder(graph);
    auto sub = builder.addGraph({}, {});

    PortId inPort1  = sub.graph.addInPort(typeId<DoubleData>());
    PortId inPort2  = sub.graph.addInPort(typeId<StringData>());
    EXPECT_TRUE(inPort1.isValid());
    EXPECT_TRUE(inPort2.isValid());
    PortId outPort1 = sub.graph.addOutPort(typeId<IntData>());
    EXPECT_TRUE(outPort1.isValid());

    ASSERT_EQ(sub.graph.ports(PortType::In).size(), 2);
    ASSERT_EQ(sub.graph.ports(PortType::Out).size(), 1);

    ASSERT_EQ(sub.graph.ports(PortType::In).size(),  sub.inNode.ports(PortType::Out).size());
    ASSERT_EQ(sub.graph.ports(PortType::Out).size(), sub.outNode.ports(PortType::In).size());

    EXPECT_EQ(sub.graph.ports(PortType::In)[0].id(),   sub.inNode.ports(PortType::Out)[0].id());
    EXPECT_EQ(sub.graph.ports(PortType::In)[0].typeId, sub.inNode.ports(PortType::Out)[0].typeId);
    EXPECT_EQ(sub.graph.ports(PortType::In)[0].typeId, typeId<DoubleData>());

    EXPECT_EQ(sub.graph.ports(PortType::In)[1].id(),   sub.inNode.ports(PortType::Out)[1].id());
    EXPECT_EQ(sub.graph.ports(PortType::In)[1].typeId, sub.inNode.ports(PortType::Out)[1].typeId);
    EXPECT_EQ(sub.graph.ports(PortType::In)[1].typeId, typeId<StringData>());

    EXPECT_EQ(sub.graph.ports(PortType::Out)[0].id(),   sub.outNode.ports(PortType::In)[0].id());
    EXPECT_EQ(sub.graph.ports(PortType::Out)[0].typeId, sub.outNode.ports(PortType::In)[0].typeId);
    EXPECT_EQ(sub.graph.ports(PortType::Out)[0].typeId, typeId<IntData>());

    sub.inNode.addPort(typeId<FileData>());

    // graph ports unchanged, only input provider changed
    ASSERT_EQ(sub.graph.ports(PortType::In).size(), 2);
    ASSERT_EQ(sub.graph.ports(PortType::Out).size(), 1);
    ASSERT_EQ(sub.inNode.ports(PortType::Out).size(), 3);
    ASSERT_EQ(sub.outNode.ports(PortType::In).size(), 1);

    sub.outNode.addPort(typeId<BoolData>());

    // graph ports unchanged, only output provider changed
    ASSERT_EQ(sub.graph.ports(PortType::In).size(), 2);
    ASSERT_EQ(sub.graph.ports(PortType::Out).size(), 1);
    ASSERT_EQ(sub.inNode.ports(PortType::Out).size(), 3);
    ASSERT_EQ(sub.outNode.ports(PortType::In).size(), 2);
}

TEST(GraphConnectioModel, iterate_over_connections)
{
    test::LinearGraph scenario{};

    Graph& graph = scenario.graph;

    auto& conModel = graph.connectionModel();

    auto iIn  = conModel.iterateConnections(scenario.cId, PortType::In);
    auto iOut = conModel.iterateConnections(scenario.cId, PortType::Out);
    auto iAll = conModel.iterateConnections(scenario.cId);
    EXPECT_EQ(iIn.size(), 2);
    EXPECT_EQ(iOut.size(), 1);
    EXPECT_EQ(iAll.size(), 3);

    auto riIn  = iIn.reverse();
    auto riOut = iOut.reverse();
    auto riAll = iAll.reverse();
    EXPECT_EQ(riIn.size(), 2);
    EXPECT_EQ(riOut.size(), 1);
    EXPECT_EQ(riAll.size(), 3);

    auto iter = iAll.begin();
    auto endIter = iAll.end();
    decltype(endIter) nullIter{};
    EXPECT_TRUE(endIter == nullIter);

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, graph.connectionId(scenario.bId, PortIndex(0),
                                          scenario.cId, PortIndex(0)));

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, graph.connectionId(scenario.bId, PortIndex(0),
                                          scenario.cId, PortIndex(1)));

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, graph.connectionId(scenario.cId, PortIndex(0),
                                          scenario.dId, PortIndex(0)));

    EXPECT_TRUE(iter == endIter);
    EXPECT_TRUE(iter == nullIter);
}

TEST(Graph, connection_model_iterate_over_connections_by_port)
{
    test::LinearGraph scenario{};

    Graph& graph = scenario.graph;

    auto& conModel = graph.connectionModel();

    auto iPort = conModel.iterateConnections(scenario.bId, PortId(2));

    EXPECT_EQ(iPort.size(), 2);
    auto riPort = iPort.reverse();
    EXPECT_EQ(riPort.size(), 2);

    auto iter = iPort.begin();
    auto endIter = iPort.end();
    decltype(endIter) nullIter{};
    EXPECT_TRUE(endIter == nullIter);

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, graph.connectionId(scenario.bId, PortIndex(0),
                                          scenario.cId, PortIndex(0)));

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, graph.connectionId(scenario.bId, PortIndex(0),
                                          scenario.cId, PortIndex(1)));

    EXPECT_TRUE(iter == endIter);
    EXPECT_TRUE(iter == nullIter);
}

TEST(Graph, connection_model_iterate_over_connected_nodes)
{
    test::LinearGraph scenario{};

    Graph& graph = scenario.graph;

    auto& conModel = graph.connectionModel();

    auto iIn  = conModel.iterateNodeIds(scenario.bId, PortType::In);
    auto iOut = conModel.iterateNodeIds(scenario.bId, PortType::Out);
    auto iAll = conModel.iterateNodeIds(scenario.bId);
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
    EXPECT_EQ(*iter++, scenario.aId);

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, scenario.cId);

    EXPECT_TRUE(iter != endIter);
    EXPECT_TRUE(iter != nullIter);
    EXPECT_EQ(*iter++, scenario.cId);

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

    auto iPort = conData->iterateNodeIds(PortId(2));
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

TEST(Graph, connection_model_iterate_over_all_connections)
{
    Graph graph;

    auto& conModel = graph.connectionModel();

    EXPECT_EQ(conModel.iterateConnections().size(), 0);
    EXPECT_EQ(graph.connections().size(), 0);

    test::buildLinearGraph(graph);

    EXPECT_EQ(conModel.iterateConnections().size(), graph.connections().size());

    graph.deleteConnection(*conModel.iterateConnections().begin());

    EXPECT_EQ(conModel.iterateConnections().size(), graph.connections().size());
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

TEST(Graph, connetion_model_reverse_iterator)
{
    QVector<int> data{1, 2, 3, 4, 5, 6};

    auto iter = utils::makeReverseIterable(data);

    ASSERT_EQ(iter.size(), data.size());
    EXPECT_TRUE(std::equal(iter.begin(), iter.end(),
                           data.rbegin(), data.rend()));
}

TEST(Graph, predessecors_and_successors)
{
    Graph graph;

    ASSERT_TRUE(test::buildBasicGraph(graph));

    EXPECT_EQ(graph.connections().size(), 5);
    EXPECT_EQ(graph.nodes().size(), 7);

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
    ASSERT_NE(E, nullptr);
    ASSERT_TRUE(graph.inputProvider());
    ASSERT_TRUE(graph.outputProvider());

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

/// Check successor and predecessor nodes of group input and output providers
/// in a graph where input and output nodes are connected to each other
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

    ASSERT_EQ(inNodeData->successors.size(), 2);
    EXPECT_EQ(inNodeData->successors.at(0).node, OUT->uuid());
    EXPECT_EQ(inNodeData->successors.at(0).port, OUT->portId(PortType::In, PortIndex(0)));
    EXPECT_EQ(inNodeData->successors.at(1).node, OUT->uuid());
    EXPECT_EQ(inNodeData->successors.at(1).port, OUT->portId(PortType::In, PortIndex(1)));

    auto outNodeData = conModel.find(OUT->uuid());
    ASSERT_NE(outNodeData, conModel.end());

    ASSERT_EQ(outNodeData->predecessors.size(), 2);
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
    EXPECT_EQ(graph.nodes().size(), 7);

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
    ASSERT_TRUE(graph.inputProvider());
    ASSERT_TRUE(graph.outputProvider());

    debug(graph);

    // delete node C

    ASSERT_TRUE(graph.deleteNode(C_id));

    EXPECT_EQ(graph.connections().size(), 2);
    EXPECT_EQ(graph.nodes().size(), 6);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(B_id), B);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(graph.findNode(C_id), nullptr);
    
    debug(graph);

    // delete node B

    ASSERT_TRUE(graph.deleteNode(B_id));

    EXPECT_EQ(graph.connections().size(), 0);
    EXPECT_EQ(graph.nodes().size(), 5);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(graph.findNode(B_id), nullptr);
    EXPECT_EQ(graph.findNode(C_id), nullptr);
    
    debug(graph);

    // delete all
    
    graph.clearGraph();

    EXPECT_EQ(graph.connections().size(), 0);
    EXPECT_EQ(graph.nodes().size(), 2);
    
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
        auto& A = builder.addNode(QStringLiteral("TestNumberInputNode")).setCaption(QStringLiteral("A"));
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

    EXPECT_EQ(graph.nodes().size(), 4);
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

    EXPECT_EQ(graph.nodes().size(), 4);
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

    auto& conModel = graph.connectionModel();

    EXPECT_EQ(graph.connections().size(), 5);
    EXPECT_EQ(graph.nodes().size(), 7);

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

    auto const checkConnectionsOfNodeC = [&graph, &conModel](){
        gtDebug() << "checking connections of node C...";

        auto consIn = conModel.iterateConnections(C_id, PortType::In); // C
        ASSERT_EQ(consIn.size(), 2);

        EXPECT_TRUE(utils::contains(consIn, graph.connectionId(A_id, PortIndex(0), C_id, PortIndex(0))));
        EXPECT_TRUE(utils::contains(consIn, graph.connectionId(B_id, PortIndex(0), C_id, PortIndex(1))));

        auto consOut = conModel.iterateConnections(C_id, PortType::Out); // C
        ASSERT_EQ(consOut.size(), 1);

        EXPECT_TRUE(utils::contains(consOut, graph.connectionId(C_id, PortIndex(0), D_id, PortIndex(0))));

        EXPECT_EQ(conModel.iterateConnections(C_id).size(), consIn.size() + consOut.size());
    };

    checkConnectionsOfNodeC();

    GtObjectMemento mementoBefore = graph.toMemento();

    // delete node c

    gtDebug() << "Deleting node C...";
    ASSERT_TRUE(graph.deleteNode(C_id));

    EXPECT_EQ(graph.connections().size(), 2);
    EXPECT_EQ(graph.nodes().size(), 6);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(B_id), B);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(graph.findNode(C_id), nullptr);

    // Node C does no longer exists -> its connections have been deleted as well
    EXPECT_EQ(conModel.iterateConnections(C_id).size(), 0);
    
    debug(graph);

    GtObjectMemento mementoAfter = graph.toMemento();

    // revert memento diff

    gtDebug() << "Reverting diff...";
    GtObjectMementoDiff diff(mementoBefore, mementoAfter);

    ASSERT_TRUE(graph.revertDiff(diff));

    // graph should have restored fully
    EXPECT_EQ(graph.connections().size(), 5);
    EXPECT_EQ(graph.nodes().size(), 7);

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
    EXPECT_EQ(graph.nodes().size(), 6);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(B_id), B);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(graph.findNode(C_id), nullptr);

    // Node C does no longer exists -> its connections have been deleted as well
    EXPECT_EQ(graph.connectionModel().iterateConnections(C_id).size(), 0);

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

    auto& conModel = graph.connectionModel();

    EXPECT_EQ(graph.connections().size(), 5);
    EXPECT_EQ(graph.nodes().size(), 7);

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

    EXPECT_EQ(conModel.iterateConnections(C_id, PortType::Out).size(), 1);
    
    debug(graph);

    GtObjectMemento mementoBefore = graph.toMemento();

    // delete node c

    ConnectionId connectionToDelete = graph.connectionId(C_id, PortIndex(0), D_id, PortIndex(0));
    ASSERT_TRUE(graph.deleteConnection(connectionToDelete));

    EXPECT_EQ(graph.connections().size(), 4);
    EXPECT_EQ(graph.nodes().size(), 7);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(B_id), B);
    EXPECT_EQ(graph.findNode(C_id), C);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(conModel.iterateConnections(C_id, PortType::Out).size(), 0);
    
    debug(graph);

    GtObjectMemento mementoAfter = graph.toMemento();

    // revert memento diff

    GtObjectMementoDiff diff(mementoBefore, mementoAfter);

    graph.revertDiff(diff);

    EXPECT_EQ(graph.connections().size(), 5);
    EXPECT_EQ(graph.nodes().size(), 7);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(B_id), B);
    EXPECT_EQ(graph.findNode(C_id), C);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(conModel.iterateConnections(C_id, PortType::Out).size(), 1);
    EXPECT_TRUE(utils::contains(conModel.iterateConnections(C_id, PortType::Out), connectionToDelete));
    
    debug(graph);

    // apply memento diff

    graph.applyDiff(diff);

    EXPECT_EQ(graph.connections().size(), 4);
    EXPECT_EQ(graph.nodes().size(), 7);

    EXPECT_EQ(graph.findNode(A_id), A);
    EXPECT_EQ(graph.findNode(B_id), B);
    EXPECT_EQ(graph.findNode(C_id), C);
    EXPECT_EQ(graph.findNode(D_id), D);
    EXPECT_EQ(graph.findNode(E_id), E);

    EXPECT_EQ(conModel.iterateConnections(C_id, PortType::Out).size(), 0);
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

    graph1.setCaption("Graph1");
    graph2.setCaption("Graph2");

    Node* nodeA = graph1.findNode(A_id);
    ASSERT_TRUE(nodeA);

    ASSERT_EQ(nodeA->parent(), &graph1);

    // before move
    EXPECT_TRUE(graph1.findNode(A_id));
    EXPECT_TRUE(graph1.findNodeByUuid(A_uuid));
    EXPECT_NE(graph2.findNode(A_id), nodeA);
    EXPECT_NE(graph2.findNodeByUuid(A_uuid), nodeA);

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

    graph1.setCaption("Graph1");
    graph2.setCaption("Graph2");

    auto model = graph1.connectionModel();
    int connections = model.size();
    auto globalModel = graph1.globalConnectionModel();
    int globalConnections = globalModel.size();

    // before move
    EXPECT_EQ(graph1.connectionModel().size(), 6);
    EXPECT_EQ(graph2.connectionModel().size(), 2);
    EXPECT_EQ(graph1.connectionModel().size(), connections);
    EXPECT_NE(graph2.connectionModel().size(), connections);

    EXPECT_EQ(graph1.globalConnectionModel().size(), 6);
    EXPECT_EQ(graph2.globalConnectionModel().size(), 2);
    EXPECT_EQ(graph1.globalConnectionModel().size(), globalConnections);
    EXPECT_NE(graph2.globalConnectionModel().size(), globalConnections);

    // move node
    QVector<NodeId> nodeIds = {A_id, B_id, C_id, D_id};
    EXPECT_TRUE(graph1.moveNodesAndConnections(nodeIds, graph2));

    // after move
    EXPECT_EQ(graph1.connectionModel().size(), 2);
    EXPECT_EQ(graph2.connectionModel().size(), 6);
    EXPECT_NE(graph1.connectionModel().size(), connections);
    EXPECT_EQ(graph2.connectionModel().size(), connections);

    EXPECT_EQ(graph1.globalConnectionModel().size(), 2);
    EXPECT_EQ(graph2.globalConnectionModel().size(), 6);
    EXPECT_NE(graph1.globalConnectionModel().size(), globalConnections);
    EXPECT_EQ(graph2.globalConnectionModel().size(), globalConnections);
}

TEST(Graph, move_graph_to_other_graph)
{
    Graph graph1;
    Graph graph2;

    ASSERT_TRUE(test::buildGraphWithGroup(graph1));

    graph1.setCaption("Graph1");
    graph2.setCaption("Graph2");

    gtDebug() << "Debugging graph1";
    debug(graph1.globalConnectionModel());
    debug(graph1.connectionModel());
    gtDebug() << "Debugging graph2";
    debug(graph2.globalConnectionModel());
    debug(graph2.connectionModel());

    auto model = graph1.connectionModel();
    int connections = model.size();
    auto globalModel = graph1.globalConnectionModel();
    int globalConnections = globalModel.size();

    // before move
    EXPECT_EQ(graph1.connectionModel().size(), 7);
    EXPECT_EQ(graph2.connectionModel().size(), 2);
    EXPECT_EQ(graph1.connectionModel().size(), connections);
    EXPECT_NE(graph2.connectionModel().size(), connections);

    EXPECT_EQ(graph1.globalConnectionModel().size(), 13);
    EXPECT_EQ(graph2.globalConnectionModel().size(), 2);
    EXPECT_EQ(graph1.globalConnectionModel().size(), globalConnections);
    EXPECT_NE(graph2.globalConnectionModel().size(), globalConnections);

    // move node
    QVector<NodeId> nodeIds = {A_id, B_id, C_id, D_id, E_id};
    EXPECT_TRUE(graph1.moveNodesAndConnections(nodeIds, graph2));

    gtDebug() << "Debugging graph1";
    debug(graph1.globalConnectionModel());
    debug(graph1.connectionModel());
    gtDebug() << "Debugging graph2";
    debug(graph2.globalConnectionModel());
    debug(graph2.connectionModel());

    // after move
    EXPECT_EQ(graph1.connectionModel().size(), 2);
    EXPECT_EQ(graph2.connectionModel().size(), 7);
    EXPECT_NE(graph1.connectionModel().size(), connections);
    EXPECT_EQ(graph2.connectionModel().size(), connections);

    EXPECT_EQ(graph1.globalConnectionModel().size(), 2);
    EXPECT_EQ(graph2.globalConnectionModel().size(), 13);
    EXPECT_NE(graph1.globalConnectionModel().size(), globalConnections);
    EXPECT_EQ(graph2.globalConnectionModel().size(), globalConnections);
}
