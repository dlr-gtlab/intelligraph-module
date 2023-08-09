/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/connection.h"

#include "intelli/private/utils.h"

#include "gt_qtutilities.h"

#include <QtNodes/Definitions>

using namespace intelli;

Connection::Connection(GtObject* parent) :
    GtObject(parent),
    m_inNodeId("inNodeId", tr("Recieving Node Id"), tr("Recieving Node Id")),
    m_inPortIdx("inPortIdx", tr("Recieving Port Idx"), tr("Recieving Port Idx")),
    m_outNodeId("outNodeId", tr("Starting Node Id"), tr("Starting Node Id")),
    m_outPortIdx("outPortIdx", tr("Starting Port Idx"), tr("Starting Port Idx"))
{
    setFlag(UserDeletable);

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

Connection::Connection(ConnectionId conId, GtObject* parent) :
    Connection(parent)
{
    fromConnectionId(conId);
}

Connection::ConnectionId
Connection::connectionId() const
{
    return ConnectionId{outNodeId(), outPortIdx(), inNodeId(), inPortIdx()};
}

bool
Connection::fromConnectionId(ConnectionId connection)
{
    m_inNodeId   = connection.inNodeId;
    m_inPortIdx  = connection.inPortIndex;
    m_outNodeId  = connection.outNodeId;
    m_outPortIdx = connection.outPortIndex;

    updateObjectName();

    return isValid();
}

NodeId
Connection::inNodeId() const
{
    return NodeId{fromInt(m_inNodeId)};
}

PortIndex
Connection::inPortIdx() const
{
    return PortIndex{fromInt(m_inPortIdx)};
}

NodeId
Connection::outNodeId() const
{
    return NodeId{fromInt(m_outNodeId)};
}

PortIndex
Connection::outPortIdx() const
{
    return PortIndex{fromInt(m_outPortIdx)};
}

bool
Connection::isValid() const
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
Connection::updateObjectName()
{
    return gt::setUniqueName(
        *this, !isValid() ? QStringLiteral("NodeConnection[N/A]") :
                            QStringLiteral("NodeConnection[%1:%2/%3:%4]")
                              .arg(m_outNodeId).arg(m_outPortIdx)
                              .arg(m_inNodeId).arg(m_inPortIdx));
}
