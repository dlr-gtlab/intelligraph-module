/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 18.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/adapter/jsonadapter.h"
#include "intelli/graph.h"
#include "intelli/node.h"
#include "intelli/connection.h"
#include "intelli/nodefactory.h"

#include "gt_objectfactory.h"
#include "gt_objectmemento.h"

#include <QJsonObject>
#include <QJsonArray>

QJsonObject
intelli::toJson(Graph const& graph, bool clone)
{
    return toJson(graph.nodes(), graph.connections(), clone);
}

QJsonObject
intelli::toJson(Node const& node, bool clone)
{
    QJsonObject json;
    json["id"] = static_cast<qlonglong>(node.id());

    auto pos = node.pos();
    QJsonObject position;
    position["x"] = pos.x();
    position["y"] = pos.y();
    json["position"] = position;

    QSize nodeSize = node.size();
    if (node.nodeFlags() & Resizable && nodeSize.isValid())
    {
        QJsonObject size;
        size["width"] = nodeSize.width();
        size["height"] = nodeSize.height();
        json["size"] = size;
    }

    auto const& memento = node.toMemento(clone);
    QJsonObject internals;
    internals["model-name"] = node.modelName();
    internals["class-name"] = memento.className();
    internals["memento"] = static_cast<QString>(memento.toByteArray());
    json["internal-data"] = internals;

    return json;
}

QJsonObject
intelli::toJson(Connection const& connection)
{
    QJsonObject json;
    json["inNodeId"]     = static_cast<qlonglong>(connection.inNodeId());
    json["inPortIndex"]  = static_cast<qlonglong>(connection.inPortIdx());
    json["outNodeId"]    = static_cast<qlonglong>(connection.outNodeId());
    json["outPortIndex"] = static_cast<qlonglong>(connection.outPortIdx());
    return json;
}

QJsonObject
intelli::toJson(QList<const Node*> const& nodes,
                QList<const Connection*> const& connections,
                bool clone)
{
    QJsonArray jConections;
    std::transform(connections.begin(), connections.end(),
                   std::back_inserter(jConections),
                   [](Connection const* con){
        return toJson(*con);
    });

    QJsonArray jNodes;
    std::transform(nodes.begin(), nodes.end(),
                   std::back_inserter(jNodes),
                   [=](Node const* node){
        return toJson(*node, clone);
    });

    QJsonObject json;
    json["connections"] = jConections;
    json["nodes"] = jNodes;

    return json;
}

std::unique_ptr<intelli::Node>
fromJsonToNode(const QJsonObject& json)
{
    using namespace intelli;

    auto internals = json["internal-data"].toObject();
    auto className = internals["class-name"].toString();

    auto node = intelli::NodeFactory::instance().newNode(className);

    node->setId(NodeId::fromValue(json["id"].toInt(invalid<NodeId>())));

    auto position = json["position"];
    node->setPos({
        position["x"].toDouble(),
        position["y"].toDouble()
    });

    auto size = json["size"];
    node->setSize({
        size["width"].toInt(-1),
        size["height"].toInt(-1)
    });

    mergeFromJson(internals, *node);

    return node;
}

std::unique_ptr<intelli::Connection>
fromJsonToConnection(const QJsonObject& json)
{
    using namespace intelli;
    
    auto connection = std::make_unique<Connection>();

    constexpr auto invalid = intelli::invalid<PortIndex>().value();

    connection->setInNodeId(  NodeId::fromValue(   json["inNodeId"].toInt(invalid)    ));
    connection->setInPortIdx( PortIndex::fromValue(json["inPortIndex"].toInt(invalid) ));
    connection->setOutNodeId( NodeId::fromValue(   json["outNodeId"].toInt(invalid)   ));
    connection->setOutPortIdx(PortIndex::fromValue(json["outPortIndex"].toInt(invalid)));

    connection->updateObjectName();

    return connection;
}

tl::optional<intelli::RestoredObjects>
fromJsonImpl(const QJsonObject& json)
{
    using namespace intelli;

    RestoredObjects objects;

    auto const& jConnections = json["connections"].toArray();

    // first we buffer the connections, as they may need to be updated
    for (auto const& jConnection : jConnections)
    {
        auto object = fromJsonToConnection(jConnection.toObject());
        if (!object->isValid())
        {
            gtWarning() << QObject::tr("Failed to restore connection:")
                        << object->objectName();
            return {};
        }
        objects.connections.push_back(std::move(object));
    }

    auto const& jNodes = json["nodes"].toArray();

    for (auto const& jNode : jNodes)
    {
        auto object = fromJsonToNode(jNode.toObject());
        if (!object->isValid())
        {
            gtWarning() << QObject::tr("Failed to restore node:")
                        << object->objectName();
            return {};
        }

        objects.nodes.push_back(std::move(object));
    }

    return objects;
}

tl::optional<intelli::RestoredObjects>
intelli::fromJson(const QJsonObject& json)
{
    try
    {
        return fromJsonImpl(json);
    }
    catch (std::exception const& e)
    {
        gtError() << QObject::tr("Failed to restore Intelli Graph from json! Error:")
                  << e.what();
        return {};
    }
}

bool
intelli::mergeFromJson(const QJsonObject& json, Node& node)
{
    auto mementoData = json["memento"].toString();

    GtObjectMemento memento(mementoData.toUtf8());

    if (memento.isNull() || !memento.mergeTo(node, *gtObjectFactory))
    {
        gtWarning()
            << QObject::tr("Failed to restore memento for '%1', "
                           "object may be incomplete")
               .arg(node.objectName());
        gtWarning().medium()
            << QObject::tr("Memento:") << mementoData;
        return false;
    }
    return true;
}