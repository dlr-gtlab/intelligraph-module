/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "test_helper.h"

#include "intelli/data/double.h"

#include "intelli/graphexecutor.h"
#include "intelli/graphdatamodel.h"
#include "intelli/node/control/conditional.h"

#include <gt_eventloop.h>

namespace intelli
{
namespace test
{


class Scenario
{
    bool success = false;

public:

    Scenario() : success(false) {}
    Scenario(Graph& g) : success(true), graph(&g) {}

    QPointer<Graph> graph;
    QMap<QString, NodeUuid> uuids;
    QMap<QString, NodeId> ids;

    void addNode(Node& node)
    {
        uuids[node.caption()] = node.uuid();
        ids[node.caption()]   = node.id();
    }

    operator bool() const { return success; }
};

inline Scenario buildConditionalGraph(Graph& graph)
{
    auto modification = graph.modify();

    GraphBuilder builder(graph);
    graph.setCaption(QStringLiteral("Root"));

    Scenario scenario(graph);

    try
    {
        auto& input = builder.addNode("intelli::DoubleInputNode")
                          .setCaption("Input");

        auto& condition = builder.addNode("intelli::BoolInputNode")
                          .setCaption("Condition");

        auto& conditional = builder.addNode<ConditionalGroupNode>();
        conditional.setCaption("Conditional");
        conditional.updateDataPort(conditional.portId(PortType::In, PortIndex(1)),
                                   makePort(typeId<DoubleData>()));
        conditional.updateDataPort(conditional.portId(PortType::Out, PortIndex(1)),
                                   makePort(typeId<DoubleData>()));

        auto& display = builder.addNode("intelli::NumberDisplayNode")
                      .setCaption("Display");

        builder.connect(condition, PortIndex(0), conditional, PortIndex(0));
        builder.connect(input, PortIndex(0), conditional, PortIndex(1));
        builder.connect(conditional, PortIndex(0), display, PortIndex(0));

        // within condition node
        GraphBuilder subBuilder{conditional};

        auto& plus = subBuilder.addNode("intelli::NumberMathNode")
                         .setCaption("Plus");
        setNodeProperty(plus, "operation", "Plus");

        auto& minus = subBuilder.addNode("intelli::NumberMathNode")
                          .setCaption("Minus");
        setNodeProperty(minus, "operation", "Minus");

        Node* conditionalIfInput = conditional.inputProvider(ConditionalGroupNode::IfBranch);
        assert(conditionalIfInput);
        Node* conditionalElseInput = conditional.inputProvider(ConditionalGroupNode::ElseBranch);
        assert(conditionalElseInput);
        Node* conditionalIfOutput = conditional.outputProvider(ConditionalGroupNode::IfBranch);
        assert(conditionalIfOutput);
        Node* conditionalElseOutput = conditional.outputProvider(ConditionalGroupNode::ElseBranch);
        assert(conditionalElseOutput);

        subBuilder.connect(*conditionalIfInput, PortIndex(0), plus, PortIndex(0));
        subBuilder.connect(*conditionalIfInput, PortIndex(0), plus, PortIndex(1));

        subBuilder.connect(*conditionalElseInput, PortIndex(0), minus, PortIndex(0));
        subBuilder.connect(*conditionalElseInput, PortIndex(0), minus, PortIndex(1));

        subBuilder.connect(plus, PortIndex(0), *conditionalIfOutput, PortIndex(0));
        subBuilder.connect(minus, PortIndex(0), *conditionalElseOutput, PortIndex(0));

        setNodeProperty(input, "value", 42.0);

        scenario.addNode(input);
        scenario.addNode(conditional);
        scenario.addNode(plus);
        scenario.addNode(minus);
        scenario.addNode(display);
    }
    catch (std::logic_error const& e)
    {
        gtError() << e.what();
        return Scenario{};
    }

    EXPECT_TRUE(isAcyclic(graph));

    return scenario;
}

} // namespace test

} // namespace intelli


TEST(GraphExecutor, test)
{
    using namespace intelli;

    Graph graph;
    GraphDataModel dataModel{graph};
    GraphExecutor executor{graph, dataModel};

    auto scenario = test::buildConditionalGraph(graph);
    ASSERT_TRUE(scenario);
    
    executor.autoEvaluate();

    gtDebug() << "scheduled!";

    GtEventLoop loop{std::chrono::seconds{1}};
    loop.exec();
}
