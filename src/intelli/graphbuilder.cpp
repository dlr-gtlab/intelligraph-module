/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/graphbuilder.h"

#include "intelli/graph.h"
#include "intelli/connection.h"
#include "intelli/nodefactory.h"

#include <gt_utilities.h>

#include <exception>

#include <intelli/node/groupinputprovider.h>
#include <intelli/node/groupoutputprovider.h>

using namespace intelli;

struct GraphBuilder::Impl
{
    Graph* graph{};
};

GraphBuilder::GraphBuilder(Graph& graph) :
    pimpl(std::make_unique<Impl>(Impl{&graph}))
{ }

GraphBuilder::~GraphBuilder() = default;

GraphBuilder::GraphData
GraphBuilder::addGraph(std::vector<PortInfo> const& inPorts,
                       std::vector<PortInfo> const& outPorts,
                       Position pos) noexcept(false)
{
    return addGraph(inPorts, outPorts, {}, {}, {}, pos);
}

GraphBuilder::GraphData
GraphBuilder::addGraph(std::vector<PortInfo> const& inPorts,
                       std::vector<PortInfo> const& outPorts,
                       NodeUuid const& graphUuid,
                       NodeUuid const& inNodeUuid,
                       NodeUuid const& outNodeUuid,
                       Position pos) noexcept(false)
{
    auto graph = std::make_unique<Graph>();
    if (!graphUuid.isEmpty()) graph->setUuid(graphUuid);

    // custom uuids for input and utput provider
    auto inputPtr  = std::make_unique<GroupInputProvider>();
    auto outputPtr = std::make_unique<GroupOutputProvider>();
    if (!inNodeUuid.isEmpty()) inputPtr->setUuid(inNodeUuid);
    if (!outNodeUuid.isEmpty()) outputPtr->setUuid(outNodeUuid);

    GroupInputProvider* input   = graph->appendNode(std::move(inputPtr));
    GroupOutputProvider* output = graph->appendNode(std::move(outputPtr));

    if (!input || !output)
    {
        throw std::logic_error{
            std::string{
                "GraphBuilder: Failed to intialize group input and output providers! " +
                gt::brackets(pimpl->graph->caption().toStdString())
            }
        };
    }

    auto success = true;
    for (auto& port : inPorts)
    {
        success &= input->insertPort(std::move(port)).isValid();
    }
    for (auto& port : outPorts)
    {
        success &= output->insertPort(std::move(port)).isValid();
    }

    if (!success)
    {
        throw std::logic_error{
            std::string{
                "GraphBuilder: Failed to insert input or output ports! " +
                gt::brackets(pimpl->graph->caption().toStdString())
            }
        };
    }

    return {
        addNode(std::move(graph), pos),
        *input,
        *output
    };
}

Node&
GraphBuilder::addNode(QString const& className, Position pos) noexcept(false)
{
    auto node = NodeFactory::instance().makeNode(className);

    return addNodeHelper(std::move(node), pos);
}

Node&
GraphBuilder::addNode(QString const& className, NodeUuid const& nodeUuid, Position pos) noexcept(false)
{
    auto node = NodeFactory::instance().makeNode(className);

    return addNodeHelper(std::move(node), pos, nodeUuid);
}

Node&
GraphBuilder::addNodeHelper(std::unique_ptr<Node> node, Position pos, NodeUuid const& nodeUuid)
{
    if (node && !pos.isNull()) node->setPos(pos);
    if (node && !nodeUuid.isEmpty()) node->setUuid(nodeUuid);

    auto* ptr = pimpl->graph->appendNode(std::move(node));

    if (!ptr)
    {
        throw std::logic_error{
            std::string{
                "GraphBuilder: Failed to append node! " +
                gt::brackets(pimpl->graph->caption().toStdString())
            }
        };
    }

    return *ptr;
}

ConnectionId
GraphBuilder::connect(Node& from, PortIndex outIdx, Node& to, PortIndex inIdx) noexcept(false)
{
    auto const buildError = [&](){
        return std::string{
            "GraphBuilder: Failed to connect node " +
            gt::squoted(from.caption().toStdString()) + " and " +
            gt::squoted(to.caption().toStdString())
        };
    };
    auto const buildPortError = [&](unsigned idx, auto str){
        return buildError() +
            ", " + str + "-going port index '" + std::to_string(idx) +
            "' is out of bounds! " +
               gt::brackets(pimpl->graph->caption().toStdString());
    };

    // check if nodes exist within the graph
    if (&from != pimpl->graph->findNode(from.id()) ||
        &to   != pimpl->graph->findNode(to.id()))
    {
        throw std::logic_error{
            buildError() +
            ", nodes have not been added to the graph before! " +
            gt::brackets(pimpl->graph->caption().toStdString())
        };
    }

    // check if out and in going ports exists
    auto* outPort = from.port(from.portId(PortType::Out, outIdx));
    if (!outPort)
    {
        throw std::logic_error{ buildPortError(outIdx, "out") };
    }
    auto* inPort  = to.port(to.portId(PortType::In, inIdx));
    if (!inPort)
    {
        throw std::logic_error{ buildPortError(inIdx, "in") };
    }

    // check if port type ids match
    if (outPort->typeId != inPort->typeId)
    {
        throw std::logic_error{
            buildError() +
            ", port type ids mismatch! " +
            gt::squoted(outPort->typeId.toStdString()) + " vs. " +
            gt::squoted(inPort->typeId.toStdString()) + " " +
            gt::brackets(pimpl->graph->caption().toStdString())
        };
    }

    auto connection = std::make_unique<Connection>();
    connection->setOutNodeId(from.id());
    connection->setOutPort(from.portId(PortType::Out, outIdx));
    connection->setInNodeId(to.id());
    connection->setInPort(to.portId(PortType::In, inIdx));

    auto conId = connection->connectionId();

    if (!pimpl->graph->appendConnection(std::move(connection)))
    {
        throw std::logic_error{
            buildError() +
            ", creating connection failed! " +
            gt::brackets(pimpl->graph->caption().toStdString())
        };
    }

    return conId;
}

ConnectionId
GraphBuilder::connect(NodeId from, PortIndex outIdx, NodeId to, PortIndex inIdx) noexcept(false)
{
    auto const buildError = [&](){
        return std::string{
            "GraphBuilder: Failed to connect node " +
            gt::squoted(std::to_string(from)) + " and " +
            gt::squoted(std::to_string(to))
        };
    };

    Node* sourceNode = pimpl->graph->findNode(from);
    Node* targetNode = pimpl->graph->findNode(to);

    // check if nodes exist within the graph
    if (!sourceNode || !targetNode)
    {
        throw std::logic_error{
            buildError() +
            ", nodes have not been added to the graph before! " +
            gt::brackets(pimpl->graph->caption().toStdString())
        };
    }

    return connect(*sourceNode, outIdx, *targetNode, inIdx);
}

void
intelli::setNodeProperty(Node& node, QString const& propertyId, QVariant value) noexcept(false)
{
    auto* property = node.findProperty(propertyId);
    if (!property)
    {
        throw std::logic_error{
            "Failed to set node property " +
            gt::squoted(propertyId.toStdString()) +
            ", property not found!"
        };
    }

    if (!property->setValueFromVariant(value))
    {
        throw std::logic_error{
            "Failed to set node property " +
            gt::squoted(propertyId.toStdString()) +
            "!"
        };
    }
}

QVariant
intelli::nodeProperty(Node& node, QString const& propertyId) noexcept(false)
{
    auto* property = node.findProperty(propertyId);
    if (!property)
    {
        throw std::logic_error{
            "Failed to get value of node property " +
            gt::squoted(propertyId.toStdString()) +
            ", property not found!"
        };
    }

    return property->valueToVariant();
}
