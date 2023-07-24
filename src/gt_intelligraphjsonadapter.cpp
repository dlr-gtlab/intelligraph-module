/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 18.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphjsonadapter.h"
#include "gt_intelligraph.h"
#include "gt_intelligraphnode.h"
#include "gt_intelligraphconnection.h"
#include "gt_intelligraphnodefactory.h"

#include "gt_objectfactory.h"
#include "gt_objectmemento.h"

#include <QJsonObject>
#include <QJsonArray>

QJsonObject
gt::ig::toJson(GtIntelliGraph const& graph, bool clone)
{
    return toJson(graph.nodes(), graph.connections(), clone);
}

QJsonObject
gt::ig::toJson(GtIntelliGraphNode const& node, bool clone)
{
    QJsonObject json;
    json["id"] = static_cast<qlonglong>(node.id());

    QJsonObject position;
    auto pos = node.pos();
    position["x"] = pos.x();
    position["y"] = pos.y();
    json["position"] = position;

    auto const& memento = node.toMemento(clone);
    QJsonObject internals;
    internals["model-name"] = memento.className();
    internals["memento"] = static_cast<QString>(memento.toByteArray());
    json["internal-data"] = internals;

    return json;
}

QJsonObject
gt::ig::toJson(GtIntelliGraphConnection const& connection)
{
    QJsonObject json;
    json["inNodeId"]     = static_cast<qlonglong>(connection.inNodeId());
    json["inPortIndex"]  = static_cast<qlonglong>(connection.inPortIdx());
    json["outNodeId"]    = static_cast<qlonglong>(connection.outNodeId());
    json["outPortIndex"] = static_cast<qlonglong>(connection.outPortIdx());
    return json;
}

QJsonObject
gt::ig::toJson(QList<const GtIntelliGraphNode*> const& nodes,
               QList<const GtIntelliGraphConnection*> const& connections,
               bool clone)
{
    QJsonArray jConections;
    std::transform(connections.begin(), connections.end(),
                   std::back_inserter(jConections),
                   [](GtIntelliGraphConnection const* con){
        return toJson(*con);
    });

    QJsonArray jNodes;
    std::transform(nodes.begin(), nodes.end(),
                   std::back_inserter(jNodes),
                   [=](GtIntelliGraphNode const* node){
        return toJson(*node, clone);
    });

    QJsonObject json;
    json["connections"] = jConections;
    json["nodes"] = jNodes;

    return json;
}

std::unique_ptr<GtIntelliGraphNode>
fromJsonToNode(const QJsonObject& json)
{
    using namespace gt::ig;

    auto internals = json["internal-data"].toObject();
    auto classname = internals["model-name"].toString();

    auto node = GtIntelliGraphNodeFactory::instance().newNode(classname);

    node->setId(NodeId::fromValue(json["id"].toInt(invalid<NodeId>())));

    auto position = json["position"];
    QPointF pos = {
        position["x"].toDouble(),
        position["y"].toDouble()
    };
    node->setPos(pos);

    mergeFromJson(internals, *node);

    return node;
}

std::unique_ptr<GtIntelliGraphConnection>
fromJsonToConnection(const QJsonObject& json)
{
    using namespace gt::ig;

    auto connection = std::make_unique<GtIntelliGraphConnection>();

    constexpr auto invalid = gt::ig::invalid<PortIndex>().value();

    connection->setInNodeId(  NodeId::fromValue(   json["inNodeId"].toInt(invalid)    ));
    connection->setInPortIdx( PortIndex::fromValue(json["inPortIndex"].toInt(invalid) ));
    connection->setOutNodeId( NodeId::fromValue(   json["outNodeId"].toInt(invalid)   ));
    connection->setOutPortIdx(PortIndex::fromValue(json["outPortIndex"].toInt(invalid)));

    connection->updateObjectName();

    return connection;
}

tl::optional<gt::ig::RestoredObjects>
fromJsonImpl(const QJsonObject& json)
{
    using namespace gt::ig;

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

tl::optional<gt::ig::RestoredObjects>
gt::ig::fromJson(const QJsonObject& json)
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
gt::ig::mergeFromJson(const QJsonObject& json, GtIntelliGraphNode& node)
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
