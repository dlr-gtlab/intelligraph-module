/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 25.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <gtest/gtest.h>

#include "intelli/graph.h"
#include "intelli/graphbuilder.h"
#include "intelli/data/double.h"
#include "intelli/graphexecmodel.h"

#include "intelli/private/utils.h"

namespace intelli
{

constexpr NodeId A_id{0};
constexpr NodeId B_id{1};
constexpr NodeId C_id{2};
constexpr NodeId D_id{3};
constexpr NodeId E_id{4};

constexpr NodeId group_id{C_id};
constexpr NodeId group_input_id{0};
constexpr NodeId group_output_id{1};
constexpr NodeId group_A_id{2};
constexpr NodeId group_B_id{3};
constexpr NodeId group_C_id{4};
constexpr NodeId group_D_id{5};

static NodeUuid A_uuid{};
static NodeUuid B_uuid{};
static NodeUuid C_uuid{};
static NodeUuid D_uuid{};
static NodeUuid E_uuid{};

static NodeUuid group_uuid{};
static NodeUuid group_input_uuid{};
static NodeUuid group_output_uuid{};

namespace test
{

inline bool buildBasicGraph(Graph& graph)
{
    GraphBuilder builder(graph);
    graph.setCaption(QStringLiteral("Root"));

    try
    {
        auto& A = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("A"));
        A_uuid = A.uuid();
        auto& B = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("B"));
        B_uuid = B.uuid();

        auto& C = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("C"));
        C_uuid = C.uuid();
        auto& D = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("D"));
        D_uuid = D.uuid();

        auto& E = builder.addNode(QStringLiteral("intelli::NumberDisplayNode")).setCaption(QStringLiteral("E"));
        E_uuid = E.uuid();

        // square value 1
        builder.connect(A, PortIndex{0}, C, PortIndex{0});
        builder.connect(B, PortIndex{0}, C, PortIndex{1});

        // multiply value 2 by result of square
        builder.connect(C, PortIndex{0}, D, PortIndex{0});
        builder.connect(B, PortIndex{0}, D, PortIndex{1});

        // forward result of add to display
        builder.connect(B, PortIndex{0}, E, PortIndex{0});

        // set values
        setNodeProperty(A, QStringLiteral("value"), 26);
        setNodeProperty(B, QStringLiteral("value"),  8);

        setNodeProperty(C, QStringLiteral("operation"), QStringLiteral("Plus"));
        setNodeProperty(D, QStringLiteral("operation"), QStringLiteral("Plus"));

        EXPECT_EQ(A.id(), A_id);
        EXPECT_EQ(B.id(), B_id);
        EXPECT_EQ(C.id(), C_id);
        EXPECT_EQ(D.id(), D_id);
        EXPECT_EQ(E.id(), E_id);
    }
    catch(std::logic_error const& e)
    {
        gtError() << "Buidling graph failed! Error:" << e.what();
        return false;
    }

    EXPECT_TRUE(isAcyclic(graph));

    return true;
}

/** basic linear graph:

  .---.      .---.      .---.
  | A |--42--| B |--42--| C |      .---.
  '---'      |   |      |   |--84--| D |
          X--|'+'|--42--|'+'|      '---'
             '---'      '---'

 */
inline bool buildLinearGraph(Graph& graph)
{
    GraphBuilder builder(graph);
    graph.setCaption(QStringLiteral("Root"));

    try
    {
        auto& A = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("A"));
        A_uuid = A.uuid();
        auto& B = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("B"));
        B_uuid = B.uuid();
        auto& C = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("C"));
        C_uuid = C.uuid();
        auto& D = builder.addNode(QStringLiteral("intelli::NumberDisplayNode")).setCaption(QStringLiteral("D"));
        D_uuid = D.uuid();

        builder.connect(A, PortIndex(0), B, PortIndex(0));

        builder.connect(B, PortIndex(0), C, PortIndex(0));
        builder.connect(B, PortIndex(0), C, PortIndex(1));

        builder.connect(C, PortIndex(0), D, PortIndex(0));

        setNodeProperty(A, QStringLiteral("value"), 42);

        setNodeProperty(B, QStringLiteral("operation"), QStringLiteral("Plus"));
        setNodeProperty(C, QStringLiteral("operation"), QStringLiteral("Plus"));

        EXPECT_EQ(A.id(), A_id);
        EXPECT_EQ(B.id(), B_id);
        EXPECT_EQ(C.id(), C_id);
        EXPECT_EQ(D.id(), D_id);
        EXPECT_EQ(A.uuid(), A_uuid);
        EXPECT_EQ(B.uuid(), B_uuid);
        EXPECT_EQ(C.uuid(), C_uuid);
        EXPECT_EQ(D.uuid(), D_uuid);
    }
    catch (std::logic_error const& e)
    {
        gtError() << e.what();
        return false;
    }

    EXPECT_TRUE(isAcyclic(graph));

    return true;
}

inline bool buildGraphWithGroup(Graph& graph)
{
    GraphBuilder builder(graph);
    graph.setCaption(QStringLiteral("Root"));

    try
    {
        auto& A = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("A"));
        A_uuid = A.uuid();
        auto& B = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("B"));
        B_uuid = B.uuid();

        auto group = builder.addGraph({
                                          typeId<DoubleData>(),
                                          typeId<DoubleData>()
                                      }, {
                                          typeId<DoubleData>()
                                      }
                                      );
        group.graph.setCaption(QStringLiteral("Group"));
        C_uuid = group.graph.uuid();

        auto& D = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("D"));
        D_uuid = D.uuid();
        auto& E = builder.addNode(QStringLiteral("intelli::NumberDisplayNode")).setCaption(QStringLiteral("E"));
        E_uuid = E.uuid();

        GraphBuilder groupBuilder(group.graph);

        auto& group_A = groupBuilder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("Group_A"));
        auto& group_B = groupBuilder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("Group_B"));
        auto& group_C = groupBuilder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("Group_C"));
        auto& group_D = groupBuilder.addNode(QStringLiteral("intelli::NumberDisplayNode")).setCaption(QStringLiteral("Group_D"));

        // square value 1
        builder.connect(A, PortIndex{0}, group.graph, PortIndex{0});
        builder.connect(B, PortIndex{0}, group.graph, PortIndex{1});

        // build group logic
        groupBuilder.connect(group_A, PortIndex{0}, group_B, PortIndex{0});
        groupBuilder.connect(group.inNode, PortIndex{0}, group_B, PortIndex{1});

        groupBuilder.connect(group_B, PortIndex{0}, group_C, PortIndex{0});
        groupBuilder.connect(group.inNode, PortIndex{1}, group_C, PortIndex{1});

        groupBuilder.connect(group_C, PortIndex{0}, group.outNode, PortIndex{0});

        groupBuilder.connect(group_C, PortIndex{0}, group_D, PortIndex{0});

        builder.connect(group.graph, PortIndex{0}, D, PortIndex{0});
        builder.connect(B, PortIndex{0}, D, PortIndex{1});

        // forward result of add to display
        builder.connect(B, PortIndex{0}, E, PortIndex{0});

        // set values
        setNodeProperty(A, QStringLiteral("value"), 26);
        setNodeProperty(B, QStringLiteral("value"),  8);
        setNodeProperty(group_A, QStringLiteral("value"), 8);

        setNodeProperty(group_B, QStringLiteral("operation"), QStringLiteral("Plus"));
        setNodeProperty(D,       QStringLiteral("operation"), QStringLiteral("Plus"));

        EXPECT_EQ(A.id(), A_id);
        EXPECT_EQ(B.id(), B_id);
        EXPECT_EQ(group.graph.id(), C_id);
        EXPECT_EQ(D.id(), D_id);
        EXPECT_EQ(E.id(), E_id);
        EXPECT_EQ(group_A.id(), group_A_id);
        EXPECT_EQ(group_B.id(), group_B_id);
        EXPECT_EQ(group_C.id(), group_C_id);
        EXPECT_EQ(group_D.id(), group_D_id);
    }
    catch(std::logic_error const& e)
    {
        gtError() << "Buidling graph failed! Error:" << e.what();
        return false;
    }

    EXPECT_TRUE(isAcyclic(graph));

    return true;
}

/** graph with a forwarding group:

  .---.          .-------.
  | A |--26------| GROUP |--26--.
  '---'          |   C   |      |
             .---|       |--O   |  .---.
             |   '-------'      '--| D |
  .---.      |                     |   |--34
  | B |---8--+---------------------| + |
  '---'      |                     '---'
             |                 .---.
             '-----------------| E |
                               '---'

Group C:
  .-----.      .-----.
  |     |--26--|     |
  | IN  |      | OUT |
  |     |---8--|     |
  '-----'      '-----'

 */
inline bool buildGraphWithForwardingGroup(Graph& graph)
{
    GraphBuilder builder(graph);
    graph.setCaption(QStringLiteral("Root"));

    try
    {
        auto& A = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("A"));
        A_uuid = A.uuid();
        auto& B = builder.addNode(QStringLiteral("intelli::NumberSourceNode")).setCaption(QStringLiteral("B"));
        B_uuid = B.uuid();

        auto group = builder.addGraph(
            {
                typeId<DoubleData>(), // forwards to 1. port of output
                typeId<DoubleData>()  // forwards to 2. port of output
            }, {
                typeId<DoubleData>(), // connected to 1. port of D
                typeId<DoubleData>()  // not connected to any port
            }
        );
        group.graph.setCaption(QStringLiteral("C"));
        C_uuid = group.graph.uuid();
        group_uuid = C_uuid;
        group_input_uuid = group.inNode.uuid();
        group_output_uuid = group.outNode.uuid();

        auto& D = builder.addNode(QStringLiteral("intelli::NumberMathNode")).setCaption(QStringLiteral("D"));
        D_uuid = D.uuid();
        auto& E = builder.addNode(QStringLiteral("intelli::NumberDisplayNode")).setCaption(QStringLiteral("E"));
        E_uuid = E.uuid();

        {
            GraphBuilder groupBuilder(group.graph);

            // build group logic
            groupBuilder.connect(group.inNode, PortIndex{0}, group.outNode, PortIndex{0});
            groupBuilder.connect(group.inNode, PortIndex{1}, group.outNode, PortIndex{1});
        }

        builder.connect(A, PortIndex{0}, group.graph, PortIndex{0});
        builder.connect(B, PortIndex{0}, group.graph, PortIndex{1});

        builder.connect(group.graph, PortIndex{0}, D, PortIndex{0});
        builder.connect(B, PortIndex{0}, D, PortIndex{1});

        // forward result of add to display
        builder.connect(B, PortIndex{0}, E, PortIndex{0});

        // set values
        setNodeProperty(A, QStringLiteral("value"), 26);
        setNodeProperty(B, QStringLiteral("value"),  8);
        setNodeProperty(D, QStringLiteral("operation"), QStringLiteral("Plus"));

        EXPECT_EQ(A.id(), A_id);
        EXPECT_EQ(B.id(), B_id);
        EXPECT_EQ(group.graph.id(), C_id);
        EXPECT_EQ(D.id(), D_id);
        EXPECT_EQ(E.id(), E_id);

        EXPECT_EQ(A.uuid(), A_uuid);
        EXPECT_EQ(B.uuid(), B_uuid);
        EXPECT_EQ(group.graph.uuid(), C_uuid);
        EXPECT_EQ(group.graph.uuid(), group_uuid);
        EXPECT_EQ(D.uuid(), D_uuid);
        EXPECT_EQ(E.uuid(), E_uuid);

        EXPECT_EQ(group.inNode.id(), group_input_id);
        EXPECT_EQ(group.outNode.id(), group_output_id);
        EXPECT_EQ(group.inNode.uuid(), group_input_uuid);
        EXPECT_EQ(group.outNode.uuid(), group_output_uuid);
    }
    catch(std::logic_error const& e)
    {
        gtError() << "Buidling graph failed! Error:" << e.what();
        return false;
    }

    EXPECT_TRUE(isAcyclic(graph));

    return true;
}

/**
 * @brief Checks the node eval state of all nodes given by `uuids`
 * @param graph Graph
 * @param model Exec model
 * @param uuids Nodes to check
 * @param targetState Eval state to check against
 * @return success
 */
inline bool compareNodeEvalState(Graph const& graph,
                                 GraphExecutionModel& model,
                                 QStringList const& uuids,
                                 NodeEvalState targetState)
{
    Q_UNUSED(graph);

    bool success = true;
    for (auto const& uuid : uuids)
    {
        if (model.nodeEvalState(uuid) != targetState)
        {
            gtError() << QObject::tr("model.nodeEvalState(%1) != %2")
                             .arg(uuid, toString(targetState));
            success = false;
        }
    }

    return success;
}

/// helper struct to compare two values of type `T`
template<typename T>
struct ValueComparator
{
    bool operator()(T const& value, T const& target)
    {
        return value == target;
    }
};

/// fuzzy compare for doubles
template<>
struct ValueComparator<double>
{
    bool operator()(double value, double target)
    {
        return std::fabs(value - target) <= std::numeric_limits<double>::epsilon();
    }
};

/// helper struct to compare a `NodeDataPtr` to a value of type `T`.
template<typename T>
struct PortDataComparator
{
    bool operator()(QString const& uuid, NodeDataPtr const& data, T const& target)
    {
        if (!data)
        {
            gtError() << QObject::tr("model.nodeData(%1).ptr == NULL")
                             .arg(uuid);
        }

        auto value = data->invoke<T>("value");
        if (!value.has_value())
        {
            gtError() << QObject::tr("model.nodeData(%1).ptr (%2) != %3 (types do not match)")
                             .arg(uuid, toString(data), toString(QVariant::fromValue(target)));
            return false;
        }

        if (!ValueComparator<T>()(value.value(), target))
        {
            gtError() << QObject::tr("model.nodeData(%1).ptr (%2) != %3")
                             .arg(uuid).arg(value.value()).arg(target);
            return false;
        }

        return true;
    }
};

// helper struct to check if data is null
template<>
struct PortDataComparator<std::nullptr_t>
{
    bool operator()(QString const& uuid, NodeDataPtr data, std::nullptr_t target)
    {
        if (data)
        {
            gtError() << QObject::tr("model.nodeData(%1).ptr (%2) != NULL")
                             .arg(uuid, toString(data));
            return false;
        }
        return true;
    }
};

/**
 * @brief Checks the data of the node given by `uuid` and its ports given by `ports`
 * @param graph Graph
 * @param model Exec model
 * @param uuid Node to check
 * @param ports Port ids to check
 * @param targetState Port state to check against
 * @param NodeDataPtr data to check against. If not explicitly given, no check is
 * performed
 * @return success
 */
template<typename T = std::nullptr_t>
inline bool comparePortData(Graph const& graph,
                            GraphExecutionModel& model,
                            QString const& uuid,
                            QVector<PortId> const& ports,
                            PortDataState targetState,
                            tl::optional<T> targetData = {})
{
    Q_UNUSED(graph);

    bool success = true;

    for (PortId portId : ports)
    {
        auto data = model.nodeData(uuid, portId);
        if (data.state != targetState)
        {
            gtError() << QObject::tr("model.nodeData(%1).state != %2")
                             .arg(uuid, toString(targetState));
            success = false;
        }

        if (!targetData.has_value()) continue;

        success &= PortDataComparator<T>()(uuid, data.ptr, targetData.value());
    }

    return success;
}

/**
 * @brief Checks the data of the node given by `uuid` and all ports of `type`
 * @param graph Graph
 * @param model Exec model
 * @param uuid Node to check
 * @param type Ports of a given type to check
 * @param targetState Port state to check against
 * @param NodeDataPtr data to check against. If not explicitly given, no check is
 * performed
 * @return success
 */
template<typename T = std::nullptr_t>
inline bool comparePortData(Graph const& graph,
                          GraphExecutionModel& model,
                          QString const& uuid,
                          PortType type,
                          PortDataState targetState,
                          tl::optional<T> targetData = {})
{
    Q_UNUSED(graph);

    auto* node = graph.findNodeByUuid(uuid);
    if (!node)
    {
        gtError() << QObject::tr("graph.findNodeByUuid(%1) == NULL")
                         .arg(uuid);
        return false;
    }

    QVector<PortId> targetPorts;
    auto const& ports = node->ports(type);

    std::transform(ports.begin(), ports.end(), std::back_inserter(targetPorts),
                   [](auto const& port){
        return port.id();
    });

    return comparePortData(graph, model, uuid, targetPorts, targetState, targetData);
}

/**
 * @brief Checks the data of the node given by `uuid` and all of its ports
 * @param graph Graph
 * @param model Exec model
 * @param uuid Node to check
 * @param targetState Port state to check against
 * @param NodeDataPtr data to check against. If not explicitly given, no check is
 * performed
 * @return success
 */
template<typename T = std::nullptr_t>
inline bool comparePortData(Graph const& graph,
                          GraphExecutionModel& model,
                          QString const& uuid,
                          PortDataState targetState,
                          tl::optional<T> targetData = {})
{
    bool success = true;
    for (auto type : {PortType::In, PortType::Out})
    {
        success &= comparePortData(graph, model, uuid, type, targetState, targetData);
    }
    return success;
}

/**
 * @brief Checks the data of all nodes given by `uuids` and all of their ports
 * @param graph Graph
 * @param model Exec model
 * @param uuids Nodes to check
 * @param targetState Port state to check against
 * @param NodeDataPtr data to check against. If not explicitly given, no check is
 * performed
 * @return success
 */
template<typename T = std::nullptr_t>
inline bool comparePortData(Graph const& graph,
                          GraphExecutionModel& model,
                          QStringList const& uuids,
                          PortDataState targetState,
                          tl::optional<T> targetData = {})
{
    bool success = true;
    for (auto const& uuid : uuids)
    {
        success &= comparePortData(graph, model, uuid, targetState, targetData);
    }

    return success;
}


} // namespace test

} // namespace intelli

#endif // TEST_HELPER_H
