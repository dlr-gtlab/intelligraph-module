/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 18.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef gt_intelligraphjsonadapter_H
#define gt_intelligraphjsonadapter_H

#include <thirdparty/tl/optional.hpp>

#include <QList>

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

QJsonObject toJson(QList<GtIntelliGraphNode const*> const& nodes,
                   QList<GtIntelliGraphConnection const*> const& connections,
                   bool clone = false);

struct RestoredObjects
{
    std::vector<std::unique_ptr<GtIntelliGraphNode>> nodes;
    std::vector<std::unique_ptr<GtIntelliGraphConnection>> connections;
};

tl::optional<RestoredObjects>
fromJson(QJsonObject const& json);

bool mergeFromJson(QJsonObject const& json, GtIntelliGraphNode& node);

} // namespace ig

} // namespace gt

#endif // GTIGJSONADPATER_H
