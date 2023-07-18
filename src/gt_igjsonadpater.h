/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 18.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_IGJSONADPATER_H
#define GT_IGJSONADPATER_H

#include <memory>

class QJsonObject;
class GtIntelliGraph;
class GtIntelliGraphNode;
class GtIntelliGraphConnection;

namespace gt
{
namespace ig
{

QJsonObject toJson(GtIntelliGraph const& graph, bool clone = false);
QJsonObject toJson(GtIntelliGraphNode const& node, bool clone = false);
QJsonObject toJson(GtIntelliGraphConnection const& connection);

bool fromJson(QJsonObject const& json, GtIntelliGraph& graph);
std::unique_ptr<GtIntelliGraphNode> fromJsonToNode(QJsonObject const& json);
std::unique_ptr<GtIntelliGraphConnection> fromJsonToConnection(QJsonObject const& json);

bool mergeFromJson(QJsonObject const& json, GtIntelliGraphNode& node);

} // namespace ig

} // namespace gt

#endif // GTIGJSONADPATER_H
