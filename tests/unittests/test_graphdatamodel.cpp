/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "test_helper.h"
#include "intelli/graphdatamodel.h"

namespace intelli
{
namespace test
{

template<typename T>
bool isDataEqual(NodeDataPtr const& data, T const& target)
{
    return PortDataComparator<T>()(QString{}, PortId{}, data, target);
}

} // namespace test

} // namespace intelli

/// accessing an input port refers to the connected input port
TEST(GraphDataModel, node_data_forwarding)
{
    using namespace intelli;

    Graph graph;

    GraphDataModel dataModel{graph};

    test::buildLinearGraph(graph);

    EXPECT_TRUE(dataModel.setNodeData(A_uuid, PortType::Out, PortIndex(0), std::make_shared<DoubleData>(42.0)));
    EXPECT_TRUE(test::isDataEqual(dataModel.nodeData(A_uuid, PortType::Out, PortIndex(0)), 42.0));

    EXPECT_TRUE(test::isDataEqual(dataModel.nodeData(B_uuid, PortType::In, PortIndex(0)), 42.0));
    EXPECT_TRUE(test::isDataEqual(dataModel.nodeData(B_uuid, PortType::In, PortIndex(1)), nullptr));

    EXPECT_TRUE(dataModel.setNodeData(B_uuid, PortType::Out, PortIndex(0), std::make_shared<DoubleData>(42.0)));
    EXPECT_TRUE(test::isDataEqual(dataModel.nodeData(B_uuid, PortType::Out, PortIndex(0)), 42.0));

    EXPECT_TRUE(test::isDataEqual(dataModel.nodeData(C_uuid, PortType::In, PortIndex(0)), 42.0));
    EXPECT_TRUE(test::isDataEqual(dataModel.nodeData(C_uuid, PortType::In, PortIndex(1)), 42.0));

    EXPECT_TRUE(dataModel.setNodeData(C_uuid, PortType::Out, PortIndex(0), std::make_shared<DoubleData>(84.0)));
    EXPECT_TRUE(test::isDataEqual(dataModel.nodeData(C_uuid, PortType::Out, PortIndex(0)), 84.0));

    EXPECT_TRUE(test::isDataEqual(dataModel.nodeData(D_uuid, PortType::In, PortIndex(0)), 84.0));
}
