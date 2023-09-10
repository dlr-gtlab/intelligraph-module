/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 9.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
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


GraphBuilder::GraphBuilder(Graph& graph) :
    m_graph(&graph)
{ }

GraphBuilder::GraphData
GraphBuilder::addGraph(std::vector<PortData> const& inPorts,
                       std::vector<PortData> const& outPorts,
                       Position pos) noexcept(false)
{
    auto graph = std::make_unique<Graph>();
    graph->initInputOutputProviders();

    auto* input = graph->inputProvider();
    auto* output = graph->outputProvider();

    if (!input || !output)
    {
        throw std::logic_error{
            std::string{
                "GraphBuilder: Failed to intialize group input and output providers! " +
                gt::brackets(m_graph->caption().toStdString())
            }
        };
    }

    auto success = true;
    for (auto& port : inPorts)
    {
        success &= input->insertPort(std::move(port));
    }
    for (auto& port : outPorts)
    {
        success &= output->insertPort(std::move(port));
    }

    if (!success)
    {
        throw std::logic_error{
            std::string{
                "GraphBuilder: Failed to insert input or output ports! " +
                gt::brackets(m_graph->caption().toStdString())
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
GraphBuilder::addNodeHelper(std::unique_ptr<Node> node, Position pos)
{
    auto* ptr = m_graph->appendNode(std::move(node));

    if (!ptr)
    {
        throw std::logic_error{
            std::string{
                "GraphBuilder: Failed to append node! " +
                gt::brackets(m_graph->caption().toStdString())
            }
        };
    }

    if (!pos.isNull()) m_graph->setNodePosition(ptr, pos);

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
            ", " + str + " going port index '" + std::to_string(idx) +
            "' is out of bounds! " +
               gt::brackets(m_graph->caption().toStdString());
    };

    // check if nodes exist within the graph
    if (&from != m_graph->findNode(from.id()) ||
        &to   != m_graph->findNode(to.id()))
    {
        throw std::logic_error{
            buildError() +
            ", nodes have not been added to the graph before! " +
            gt::brackets(m_graph->caption().toStdString())
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
            gt::brackets(m_graph->caption().toStdString())
        };
    }

    auto connection = std::make_unique<Connection>();
    connection->setOutNodeId(from.id());
    connection->setOutPort(from.portId(PortType::Out, outIdx));
    connection->setInNodeId(to.id());
    connection->setInPort(to.portId(PortType::In, inIdx));

    auto conId = connection->connectionId();

    if (!m_graph->appendConnection(std::move(connection)))
    {
        throw std::logic_error{
            buildError() +
            ", creating connection failed! " +
            gt::brackets(m_graph->caption().toStdString())
        };
    }

    return conId;
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
