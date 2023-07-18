/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphconnection.h"

#include "gt_qtutilities.h"

GtIntelliGraphConnection::GtIntelliGraphConnection(GtObject* parent) :
    GtObject(parent),
    m_inNodeId("inNodeId", tr("Starting Node Id"), tr("Starting Node Id")),
    m_inPortIdx("inPortIdx", tr("Starting Port Idx"), tr("Starting Port Idx")),
    m_outNodeId("outNodeId", tr("Recieving Node Id"), tr("Recieving Node Id")),
    m_outPortIdx("outPortIdx", tr("Recieving Port Idx"), tr("Recieving Port Idx"))
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
}

GtIntelliGraphConnection::QtConnectionId
GtIntelliGraphConnection::toConnectionId() const
{
    return QtConnectionId{outNodeId(), outPortIdx(), inNodeId(), inPortIdx()};
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
