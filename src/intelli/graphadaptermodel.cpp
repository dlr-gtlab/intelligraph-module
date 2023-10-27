/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 18.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/graphadaptermodel.h"

#include "intelli/connection.h"
#include "intelli/node.h"
#include "intelli/nodedatafactory.h"
#include "intelli/graph.h"
#include "intelli/private/utils.h"

#include <gt_coreapplication.h>

#include <QtNodes/NodeData>

#define NOT_IMPLEMENTED gtWarning() << tr("Function '%1' not implemented!").arg(__FUNCTION__)

/// https://stackoverflow.com/questions/27429705/std-insert-iterator-for-unordered-sets-or-maps
template<typename Container>
class map_inserter_iterator {
public:
    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using reference_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;
    using container_type = Container;

    map_inserter_iterator& operator++() {return *this;} //no-op
    map_inserter_iterator& operator++(int) {return *this;} //no-op
    map_inserter_iterator& operator*() {return *this;} //no-op
    constexpr map_inserter_iterator& operator=(const typename Container::value_type& value) {
        container->insert(value);
        return *this;
    }
    constexpr map_inserter_iterator& operator=(typename Container::value_type&& value) {
        container->insert(std::move(value));
        return *this;
    }
    map_inserter_iterator(Container* container)
        : container(container)
    {}
protected:
    Container* container;
};

template <typename T>
auto map_inserter(T& container)
{
    return map_inserter_iterator<T>(&container);
}

using namespace intelli;

GraphAdapterModel::GraphAdapterModel(Graph& graph)
{
    if (graph.findChild<GraphAdapterModel*>())
    {
        gtError() << tr("Graph '%1' already has an adapter model!")
                         .arg(graph.objectName());
    }

    setObjectName("__adapter_model");
    setParent(&graph);

    m_graph = &graph;

    auto const onNodeAppended = [this](Node* node){
        assert(node);

        connect(node, &Node::nodeStateChanged,
                this, &GraphAdapterModel::onNodeEvalStateUpdated,
                Qt::UniqueConnection);

        connect(node, &Node::nodeChanged,
                this, &GraphAdapterModel::onNodeChanged,
                Qt::UniqueConnection);
        connect(node, &Node::portChanged,
                this, &GraphAdapterModel::onNodeChanged,
                Qt::UniqueConnection);

        connect(node, &Node::portAboutToBeInserted,
                this, &GraphAdapterModel::onPortAboutToBeInserted,
                Qt::UniqueConnection);
        connect(node, &Node::portInserted,
                this, &GraphAdapterModel::onPortInserted,
                Qt::UniqueConnection);

        connect(node, &Node::portAboutToBeDeleted,
                this, &GraphAdapterModel::onPortAboutToBeDeleted,
                Qt::UniqueConnection);
        connect(node, &Node::portDeleted,
                this, &GraphAdapterModel::onPortDeleted,
                Qt::UniqueConnection);

        Geometry geo;
        geo.pos  = node->pos();
        geo.size = QSize();

        m_geometries.insert(node->id(), std::move(geo));

        emit nodeCreated(node->id());
    };

    auto const onConnectionAppended = [this](Connection* connection){
        assert(connection);
        emit connectionCreated(convert(connection->connectionId()));
    };

    connect(&graph, &Graph::nodeAppended, this, onNodeAppended);
    connect(&graph, &Graph::nodeDeleted, this, [this](NodeId nodeId){
        m_geometries.remove(nodeId);
        emit nodeDeleted(nodeId);
    });
    connect(&graph, &Graph::connectionAppended, this, onConnectionAppended);
    connect(&graph, &Graph::connectionDeleted, this, [this](ConnectionId conId){
        emit connectionDeleted(convert(conId));
    });

    // init model
    for (auto* node : graph.nodes())       onNodeAppended(node);
    for (auto* con  : graph.connections()) onConnectionAppended(con);
}

GraphAdapterModel::~GraphAdapterModel()
{
    gtTrace().verbose() << __FUNCTION__;
}

Graph&
GraphAdapterModel::graph()
{
    assert(m_graph);
    return *m_graph;
}

const Graph&
GraphAdapterModel::graph() const
{
    return const_cast<GraphAdapterModel*>(this)->graph();
}

QtNodes::ConnectionId
GraphAdapterModel::convert(ConnectionId conId) const
{
    auto& graph = this->graph();

    auto* outNode = graph.findNode(conId.outNodeId);
    auto* inNode  = graph.findNode(conId.inNodeId);

    if (!outNode || !inNode)
    {
        gtError() << tr("Failed to convert connection %1, in or out node not found!")
                         .arg(toString(conId));
        return { invalid<NodeId>(), invalid<PortIndex>(), invalid<NodeId>(), invalid<PortIndex>() };
    }

    auto outPort = outNode->portIndex(PortType::Out, conId.outPort);
    auto inPort  =  inNode->portIndex(PortType::In,  conId.inPort);

    if (outPort == invalid<PortIndex>() || inPort == invalid<PortIndex>())
    {
        gtError() << tr("Failed to convert connection %1, invalid in or out port!")
                         .arg(toString(conId))
                  << tr("Outport: %1, Inport: %2").arg(outPort).arg(inPort);
        return { invalid<NodeId>(), invalid<PortIndex>(), invalid<NodeId>(), invalid<PortIndex>() };
    }

    return {
        outNode->id(),
        outNode->portIndex(PortType::Out, conId.outPort),
        inNode->id(),
        inNode->portIndex(PortType::In, conId.inPort),
    };
}

ConnectionId
GraphAdapterModel::convert(QtNodes::ConnectionId conId) const
{
    auto& graph = this->graph();

    NodeId inNode(conId.inNodeId);
    NodeId outNode(conId.outNodeId);
    PortIndex inPortIdx(conId.inPortIndex);
    PortIndex outPortIdx(conId.outPortIndex);

    ConnectionId converted{
        outNode,
        graph.portId(outNode, PortType::Out, outPortIdx),
        inNode,
        graph.portId(inNode, PortType::In, inPortIdx),
    };

    return converted.isValid() ? converted : invalid<ConnectionId>();
}

QtNodes::NodeId
GraphAdapterModel::newNodeId()
{
    NOT_IMPLEMENTED;
    return invalid<NodeId>();
}

std::unordered_set<QtNodes::NodeId>
GraphAdapterModel::allNodeIds() const
{
    std::unordered_set<QtNodes::NodeId> set;

    auto const& nodes = graph().nodes();
    std::transform(nodes.begin(), nodes.end(), map_inserter(set), [](Node const* node){
        return node->id();
    });

    return set;
}

bool
GraphAdapterModel::nodeExists(QtNodes::NodeId nodeId) const
{
    return graph().findNode(NodeId(nodeId));
}

std::unordered_set<QtNodes::ConnectionId>
GraphAdapterModel::allConnectionIds(QtNodes::NodeId nodeId) const
{
    std::unordered_set<QtNodes::ConnectionId> set;

    auto const& connections = graph().findConnections(NodeId(nodeId));
    std::transform(connections.begin(), connections.end(), map_inserter(set), [this](ConnectionId conId){
        return convert(conId);
    });

    return set;
}

std::unordered_set<QtNodes::ConnectionId>
GraphAdapterModel::connections(QtNodes::NodeId nodeId, QtNodes::PortType portType, QtNodes::PortIndex index) const
{
    std::unordered_set<QtNodes::ConnectionId> set;

    auto& graph = this->graph();
    auto portId = graph.portId(NodeId(nodeId), ::convert(portType), PortIndex(index));

    auto const& connections = graph.findConnections(NodeId(nodeId), portId);
    std::transform(connections.begin(), connections.end(), map_inserter(set), [this](ConnectionId conId){
        return convert(conId);
    });

    return set;
}

bool
GraphAdapterModel::connectionExists(QtNodes::ConnectionId connectionId) const
{
    return graph().findConnection(convert(connectionId));
}

QtNodes::NodeId
GraphAdapterModel::addNode(const QString& nodeType)
{
    NOT_IMPLEMENTED;
    return invalid<NodeId>();
}

void
GraphAdapterModel::addConnection(QtNodes::ConnectionId connectionId)
{
    auto conId = convert(connectionId);
    auto& graph = this->graph();

    if (graph.findConnection(conId)) return;

    auto command = gtApp->makeCommand(&graph, tr("Appending %1").arg(toString(conId)));
    Q_UNUSED(command);

    if (!graph.appendConnection(std::make_unique<Connection>(conId)))
    {
        emit connectionDeleted(connectionId);
    }
}

bool
GraphAdapterModel::connectionPossible(QtNodes::ConnectionId connectionId) const
{
    auto& graph = this->graph();
    auto conId = convert(connectionId);

    bool connectionAlreadyExists = graph.findConnection(conId);
    if (connectionAlreadyExists) return false;

    auto* outNode = graph.findNode(conId.outNodeId);
    auto* inNode  = graph.findNode(conId.inNodeId);

    bool nodeExists = outNode && inNode;
    if (!nodeExists) return false;

    bool isInputConnected = !graph.findConnections(conId.inNodeId, conId.inPort).empty();
    if (isInputConnected) return false;

    auto* outPort = outNode->port(conId.outPort);
    auto* inPort  = inNode->port(conId.inPort);

    bool portsExist = outPort && inPort;
    if (!portsExist) return false;

    bool dataTypesCompatible = outPort->typeId == inPort->typeId;
    if (!dataTypesCompatible) return false;

    return true;
}

QVariant
GraphAdapterModel::nodeData(QtNodes::NodeId nodeId, QtNodes::NodeRole role) const
{
    auto& graph = this->graph();
    auto* node = graph.findNode(NodeId(nodeId));
    if (!node) return {};

    switch(role)
    {
    case QtNodes::NodeRole::Type:
        return node->modelName();
    case QtNodes::NodeRole::Position:
        return m_geometries[node->id()].pos;
    case QtNodes::NodeRole::Size:
        return m_geometries[node->id()].size;
    case QtNodes::NodeRole::Caption:
        return node->caption();
    case QtNodes::NodeRole::CaptionVisible:
        return !(node->nodeFlags() & NodeFlag::HideCaption);
    case QtNodes::NodeRole::InternalData:
        return {};
    case QtNodes::NodeRole::InPortCount:
        return (qulonglong)node->ports(PortType::In).size();
    case QtNodes::NodeRole::OutPortCount:
        return (qulonglong)node->ports(PortType::Out).size();
    case QtNodes::NodeRole::Widget:
        return QVariant::fromValue(const_cast<Node*>(node)->embeddedWidget());
    }

    gtError() << tr("Invalid node role!") << role;
    return {};
}

bool
GraphAdapterModel::setNodeData(QtNodes::NodeId nodeId, QtNodes::NodeRole role, QVariant value)
{
    switch(role)
    {
    case QtNodes::NodeRole::Type:
    case QtNodes::NodeRole::CaptionVisible:
    case QtNodes::NodeRole::InternalData:
    case QtNodes::NodeRole::InPortCount:
    case QtNodes::NodeRole::OutPortCount:
    case QtNodes::NodeRole::Widget:
        NOT_IMPLEMENTED;
        return false;
    default:
        break;
    }

    auto& graph = this->graph();
    auto* node = graph.findNode(NodeId(nodeId));
    if (!node) return {};

    switch(role)
    {
    case QtNodes::NodeRole::Position:
    {
        auto pos = value.toPointF();
        m_geometries[node->id()].pos = pos;
        emit nodePositionUpdated(nodeId);
        return true;
    }
    case QtNodes::NodeRole::Size:
    {
        auto size = value.toSize();
        if (!size.isValid()) return false;
        m_geometries[node->id()].size = size;
        return true;
    }
    case QtNodes::NodeRole::Caption:
    {
        auto name = value.toString();
        if (name.isEmpty()) return false;
        node->setCaption(name);
        return true;
    }
    }

    gtError() << tr("Invalid node role!") << role;
    return {};
}

QtNodes::NodeFlags
GraphAdapterModel::nodeFlags(QtNodes::NodeId nodeId) const
{
    QtNodes::NodeFlags flags = QtNodes::NodeFlag::NoFlags;

    auto& graph = this->graph();
    auto* node = graph.findNode(NodeId(nodeId));
    if (!node) return flags;

    auto sourceFlags = node->nodeFlags();

    if (sourceFlags & NodeFlag::Resizable)
    {
        flags.setFlag(QtNodes::NodeFlag::Resizable);
    }
    if (sourceFlags & NodeFlag::Unique)
    {
        flags.setFlag(QtNodes::NodeFlag::Unique);
    }
    if (node->objectFlags() & GtObject::UserDeletable)
    {
        flags.setFlag(QtNodes::NodeFlag::Deletable);
    }

    return flags;
}

QtNodes::NodeEvalState
GraphAdapterModel::nodeEvalState(QtNodes::NodeId nodeId) const
{
    auto* node = graph().findNode(NodeId(nodeId));
    if (!node)
    {
        return QtNodes::NodeEvalState::NoState;
    }

    if (node->nodeFlags() & NodeFlag::Evaluating)
    {
        return QtNodes::NodeEvalState::Evaluating;
    }

    return node->isActive() ? QtNodes::NodeEvalState::NoState :
                              QtNodes::NodeEvalState::Paused;
}

QVariant
GraphAdapterModel::portData(QtNodes::NodeId nodeId, QtNodes::PortType portType, QtNodes::PortIndex index, QtNodes::PortRole role) const
{
    auto& graph = this->graph();
    auto* node = graph.findNode(NodeId(nodeId));
    if (!node) return {};

    auto* port = node->port(node->portId(::convert(portType), PortIndex(index)));
    if (!port) return {};

    auto& factory = NodeDataFactory::instance();

    switch(role)
    {
    case QtNodes::PortRole::Data:
        NOT_IMPLEMENTED;
        return {};
    case QtNodes::PortRole::DataType:
        return QVariant::fromValue(
            QtNodes::NodeDataType{
                port->typeId, factory.typeName(port->typeId)
            }
        );
    case QtNodes::PortRole::ConnectionPolicyRole:
        return (int) (portType == QtNodes::PortType::In ?
                          QtNodes::ConnectionPolicy::One :
                          QtNodes::ConnectionPolicy::Many);
    case QtNodes::PortRole::CaptionVisible:
        return port->captionVisible;
    case QtNodes::PortRole::Caption:
    {
        if (!port->captionVisible) return {};
        auto const& name = factory.typeName(port->typeId);
        if (port->caption.isEmpty()) return name;
        return port->caption + QChar('\n') + QChar('(') + name + QChar(')');
    }
    }

    gtError() << tr("Invalid port role!") << role;
    return {};
}

bool
GraphAdapterModel::setPortData(QtNodes::NodeId nodeId, QtNodes::PortType portType, QtNodes::PortIndex index, const QVariant& value, QtNodes::PortRole role)
{
    switch(role)
    {
    case QtNodes::PortRole::Data:
    case QtNodes::PortRole::DataType:
    case QtNodes::PortRole::ConnectionPolicyRole:
    case QtNodes::PortRole::CaptionVisible:
    case QtNodes::PortRole::Caption:
        NOT_IMPLEMENTED;
        return false;
    }

    gtError() << tr("Invalid port role!") << role;
    return false;
}

bool
GraphAdapterModel::deleteConnection(QtNodes::ConnectionId connectionId)
{
    auto conId = convert(connectionId);
    // don't delete the connections that are beeing moved
    if (m_shiftedConnections.contains(conId)) return false;

    auto& graph = this->graph();

    auto command = gtApp->makeCommand(&graph, tr("Deleting %1").arg(toString(conId)));
    Q_UNUSED(command);

    return graph.deleteConnection(conId);
}

bool
GraphAdapterModel::deleteNode(const QtNodes::NodeId nodeId)
{
    NOT_IMPLEMENTED;
    return false;
}

void
GraphAdapterModel::commitPosition(NodeId nodeId)
{
    auto& graph = this->graph();
    if (auto* node = graph.findNode(NodeId(nodeId)))
    {
        node->setPos(m_geometries[nodeId].pos);
    }
}

void
GraphAdapterModel::onNodeEvalStateUpdated()
{
    auto* node = qobject_cast<Node*>(sender());
    if (!node) return;

    emit nodeEvalStateUpdated(node->id());
}

void
GraphAdapterModel::onNodeChanged()
{
    auto* node = qobject_cast<Node*>(sender());
    if (!node) return;

    emit nodeUpdated(node->id());
}

void
GraphAdapterModel::onPortAboutToBeInserted(PortType type, PortIndex idx)
{
    auto* node = qobject_cast<Node*>(sender());
    if (!node) return;

    assert(m_shiftedConnections.empty());
    m_shiftedConnections = graph().findConnections(node->id(), node->portId(type, idx));

    portsAboutToBeInserted(node->id(), ::convert(type), idx, idx);
}

void
GraphAdapterModel::onPortInserted(PortType type, PortIndex idx)
{
    Q_UNUSED(type);
    Q_UNUSED(idx);

    auto* node = qobject_cast<Node*>(sender());
    if (!node) return;

    m_shiftedConnections.clear();

    portsInserted();

    emit nodeUpdated(node->id());
}

void
GraphAdapterModel::onPortAboutToBeDeleted(PortType type, PortIndex idx)
{
    auto* node = qobject_cast<Node*>(sender());
    if (!node) return;

    assert(m_shiftedConnections.empty());
    m_shiftedConnections = graph().findConnections(node->id(), node->portId(type, idx));

    portsAboutToBeDeleted(node->id(), ::convert(type), idx, idx);
}

void
GraphAdapterModel::onPortDeleted(PortType type, PortIndex idx)
{
    Q_UNUSED(type);
    Q_UNUSED(idx);

    auto* node = qobject_cast<Node*>(sender());
    if (!node) return;

    m_shiftedConnections.clear();

    portsDeleted();

    emit nodeUpdated(node->id());
}
