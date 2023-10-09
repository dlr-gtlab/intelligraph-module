/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/nodedatainterface.h"
#include "intelli/node.h"
#include "intelli/graph.h"

using namespace intelli;

bool
dm::Entry::isEvaluated(Node const& node) const
{
    return !(node.nodeFlags() & (NodeFlag::Evaluating | NodeFlag::RequiresEvaluation)) &&
           std::all_of(portsOut.begin(), portsOut.end(), [](auto const& p){
               return p.state == PortDataState::Valid;
           });
}

bool
dm::Entry::areInputsValid(Graph const& graph, NodeId nodeId) const
{
    bool valid = std::all_of(portsIn.begin(), portsIn.end(),
                             [&graph, nodeId](auto const& p){
        return graph.findConnections(nodeId, p.id).empty() ||
               p.state == PortDataState::Valid;
    });
    return valid;
}

bool
dm::Entry::canEvaluate(Graph const& graph, Node const& node) const
{
    auto const& nodePorts = node.ports(PortType::In);
    assert((size_t)portsIn.size() == nodePorts.size());

    return areInputsValid(graph, node.id()) &&
           std::all_of(portsIn.begin(), portsIn.end(),
                       [&](dm::PortEntry const& port){
                           auto* p = node.port(port.id);
                           return (p && p->optional) || port.data;
                       });
}

