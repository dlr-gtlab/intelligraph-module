/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 9.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_GRAPHBUILDER_H
#define GT_INTELLI_GRAPHBUILDER_H

#include <intelli/node.h>
#include <intelli/dynamicnode.h>

#include <gt_typetraits.h>

namespace intelli
{

class Graph;

GT_INTELLI_EXPORT
void setNodeProperty(Node& node, QString const& propertyId, QVariant value) noexcept(false);

GT_INTELLI_EXPORT
QVariant nodeProperty(Node& node, QString const& propertyId) noexcept(false);

template <typename T>
inline T nodeProperty(Node& node, QString const& propertyId) noexcept(false)
{
    return nodeProperty(node, propertyId).value<T>();
}

class GT_INTELLI_EXPORT GraphBuilder
{
public:

    using PortInfo = Node::PortInfo;
    using PortData [[deprecated("use PortInfo")]] = PortInfo;

    GraphBuilder(Graph& graph);

    GraphBuilder(GraphBuilder const&) = delete;
    GraphBuilder(GraphBuilder&&) = default;
    GraphBuilder& operator=(GraphBuilder const&) = delete;
    GraphBuilder& operator=(GraphBuilder&&) = default;
    ~GraphBuilder() = default;

    /**
     * Helper struct for accessing graph data
     */
    struct GraphData
    {
        Graph& graph;
        DynamicNode& inNode;
        DynamicNode& outNode;
    };

    /**
     * @brief Adds a graph with the desired input and output nodes
     * @param inPorts
     * @param outPorts
     * @param pos Optional: position in graph
     * @return
     */
    GraphData addGraph(std::vector<PortInfo> const& inPorts,
                       std::vector<PortInfo> const& outPorts,
                       Position pos = {}) noexcept(false);
    GraphData addGraph(std::vector<PortInfo> const& inPorts,
                       std::vector<PortInfo> const& outPorts,
                       NodeUuid const& graphUuid,
                       NodeUuid const& inNodeUuid,
                       NodeUuid const& outNodeUUuid,
                       Position pos = {}) noexcept(false);

    /**
     * @brief Adds the node to the intelli graph beeing built. The node is not
     * connected by default. If the node cannot be appended (e.g. becuase the
     * node is not registered, an exception is thrown)
     * @param className Class name to create node from
     * @param pos Optional: position in graph
     * @return Pointer to node (never null)
     */
    Node& addNode(QString const& className, Position pos = {}) noexcept(false);
    Node& addNode(QString const& className, NodeUuid const& nodeUuid, Position pos = {}) noexcept(false);

    /**
     * @brief Overloads. Deduces class name by template type.
     * @param pos Optional: position in graph
     * @return Pointer to node (never null)
     */
    template <typename Derived,
              gt::trait::enable_if_base_of<Node, Derived> = true>
    Derived& addNode(Position pos = {}) noexcept(false)
    {
        return addNode<Derived>(NodeUuid{}, pos);
    }
    template <typename Derived,
              gt::trait::enable_if_base_of<Node, Derived> = true>
    Derived& addNode(NodeUuid const& nodeUuid, Position pos = {}) noexcept(false)
    {
        return static_cast<Derived&>(addNode(QString{Derived::staticMetaObject.className()}, nodeUuid, pos));
    }

    /**
     * @brief Overload. Adds the node to the intelli graph beeing built. The
     * node is not connected by default. If the node cannot be appended (e.g.
     * because it is null, an exception is thrown)
     * @param node Node to append
     * @param pos Optional: position in graph
     * @return Pointer to node (never null)
     */
    template <typename Derived,
              gt::trait::enable_if_base_of<Node, Derived> = true>
    Derived& addNode(std::unique_ptr<Derived> node, Position pos = {}) noexcept(false)
    {
        return static_cast<Derived&>(addNodeHelper(std::move(node)));
    }

    /**
     * @brief Connects two nodes and their output and input ports respectively.
     * If a port index is out of range or the connection cannot be established,
     * an exception is thrown.
     * @param from Node to begin the connection from
     * @param outIdx Output port of starting node to begin the connection from
     * @param to Node to end connection at
     * @param inIdx Input port of the end node to end the connection at
     * @return connectionId
     */
    ConnectionId connect(Node& from, PortIndex outIdx, Node& to, PortIndex inIdx) noexcept(false);

    /**
     * @brief Overload. Connects two nodes based on their id.
     * @param from Node to begin the connection from
     * @param outIdx Output port of starting node to begin the connection from
     * @param to Node to end connection at
     * @param inIdx Input port of the end node to end the connection at
     * @return connectionId
     */
    ConnectionId connect(NodeId from, PortIndex outIdx, NodeId to, PortIndex inIdx) noexcept(false);

private:

    Graph* m_graph;

    Node& addNodeHelper(std::unique_ptr<Node> node, Position pos = {}, NodeUuid const& nodeUuid = {});
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHBUILDER_H
