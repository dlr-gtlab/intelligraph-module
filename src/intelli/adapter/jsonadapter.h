/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 18.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_JSONADAPTER_H
#define GT_INTELLI_JSONADAPTER_H

#include <intelli/node.h>
#include <intelli/connection.h>

#include <thirdparty/tl/optional.hpp>

#include <QList>

#include <memory>

class QJsonObject;

namespace intelli
{

class Graph;

QJsonObject toJson(Graph const& graph, bool clone = false);
QJsonObject toJson(Node const& node, bool clone = false);
QJsonObject toJson(Connection const& connection);

QJsonObject toJson(QList<Node const*> const& nodes,
                   QList<Connection const*> const& connections,
                   bool clone = false);

struct RestoredObjects
{
    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<std::unique_ptr<Connection>> connections;
};

tl::optional<RestoredObjects>
fromJson(QJsonObject const& json);

bool mergeFromJson(QJsonObject const& json, Node& node);

} // namespace intelli

#endif // GT_INTELLI_JSONADAPTER_H
