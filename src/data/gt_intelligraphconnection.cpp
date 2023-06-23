/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphconnection.h"

#include "gt_igglobals.h"
#include "gt_qtutilities.h"
#include "gt_application.h"

#include <QtNodes/ConnectionIdUtils>

GtIntelliGraphConnection::GtIntelliGraphConnection(GtObject* parent) :
    GtObject(parent),
    m_inNodeId("inNodeId", tr("Ingoing Node Id"), tr("Ingoing Node Id")),
    m_inPortIdx("inPortIdx", tr("Ingoing Port Idx"), tr("Ingoing Port Idx")),
    m_outNodeId("outNodeId", tr("Outgoing Node Id"), tr("Outgoing Node Id")),
    m_outPortIdx("outPortIdx", tr("Outgoing Port Idx"), tr("Outgoing Port Idx"))
{
    static const QString cat = QStringLiteral("Node");
    registerProperty(m_inNodeId, cat);
    registerProperty(m_inPortIdx, cat);
    registerProperty(m_outNodeId, cat);
    registerProperty(m_outPortIdx, cat);

    m_inNodeId.setReadOnly(true);
    m_inPortIdx.setReadOnly(true);
    m_outNodeId.setReadOnly(true);
    m_outPortIdx.setReadOnly(true);

    updateObjectName();

    setFlag(UserDeletable);
    if (!gtApp || !gtApp->devMode()) setFlag(UserHidden);
}

bool
GtIntelliGraphConnection::fromConnectionId(QtConnectionId connection)
{
    m_inNodeId   = connection.inNodeId;
    m_inPortIdx  = connection.inPortIndex;
    m_outNodeId  = connection.outNodeId;
    m_outPortIdx = connection.outPortIndex;

    updateObjectName();

    return isValid();
}

bool
GtIntelliGraphConnection::fromJson(const QJsonObject& json)
{
    constexpr auto invalid = QtNodes::InvalidPortIndex;

    m_inNodeId   = json["inNodeId"].toInt(invalid);
    m_inPortIdx  = json["inPortIndex"].toInt(invalid);
    m_outNodeId  = json["outNodeId"].toInt(invalid);
    m_outPortIdx = json["outPortIndex"].toInt(invalid);

    updateObjectName();

    return isValid();
}

QJsonObject
GtIntelliGraphConnection::toJson() const
{
    QJsonObject json;
    json["inNodeId"]     = m_inNodeId.get();
    json["inPortIndex"]  = m_inPortIdx.get();
    json["outNodeId"]    = m_outNodeId.get();
    json["outPortIndex"] = m_outPortIdx.get();
    return json;
}

bool
GtIntelliGraphConnection::isValid() const
{
    constexpr auto invalid = QtNodes::InvalidPortIndex;

    std::array<unsigned, 4> const ids{
        inNodeId(), inPortIdx(), outNodeId(), outPortIdx()
    };
    return std::all_of(std::begin(ids), std::end(ids), [=](auto id){
        return id != invalid;
    });
}

void
GtIntelliGraphConnection::updateObjectName()
{
    return gt::setUniqueName(
        *this, !isValid() ? QStringLiteral("NodeConnection[N/A]") :
                            QStringLiteral("NodeConnection[%1:%2/%3:%4]")
                                .arg(m_inNodeId).arg(m_inPortIdx)
                                .arg(m_outNodeId).arg(m_outPortIdx));
}
