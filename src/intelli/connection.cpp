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

using namespace intelli;

Connection::Connection(GtObject* parent) :
    GtObject(parent),
    m_inNodeId("inNodeId", tr("Recieving Node Id"), tr("Recieving Node Id")),
    m_inPort("inPort", tr("Recieving Port Idx"), tr("Recieving Port Idx")),
    m_outNodeId("outNodeId", tr("Starting Node Id"), tr("Starting Node Id")),
    m_outPort("outPort", tr("Starting Port Idx"), tr("Starting Port Idx"))
{
    setFlag(UserDeletable);

    static const QString cat = QStringLiteral("Node");
    registerProperty(m_inNodeId, cat);
    registerProperty(m_inPort, cat);
    registerProperty(m_outNodeId, cat);
    registerProperty(m_outPort, cat);

    m_inNodeId.setReadOnly(true);
    m_inPort.setReadOnly(true);
    m_outNodeId.setReadOnly(true);
    m_outPort.setReadOnly(true);

    updateObjectName();
}

Connection::Connection(ConnectionId conId, GtObject* parent) :
    Connection(parent)
{
    fromConnectionId(conId);
}

intelli::ConnectionId
Connection::connectionId() const
{
    return ConnectionId{outNodeId(), outPort(), inNodeId(), inPort()};
}

bool
Connection::fromConnectionId(ConnectionId connection)
{
    m_inNodeId  = connection.inNodeId;
    m_inPort    = connection.inPort;
    m_outNodeId = connection.outNodeId;
    m_outPort   = connection.outPort;

    updateObjectName();

    return isValid();
}

NodeId
Connection::inNodeId() const
{
    return NodeId{fromInt(m_inNodeId)};
}

PortId
Connection::inPort() const
{
    return PortId{fromInt(m_inPort)};
}

NodeId
Connection::outNodeId() const
{
    return NodeId{fromInt(m_outNodeId)};
}

PortId
Connection::outPort() const
{
    return PortId{fromInt(m_outPort)};
}

bool
Connection::isValid() const
{
    constexpr auto invalid = intelli::invalid<PortId>().value();

    std::array<unsigned, 4> const ids{
                                      inNodeId(), inPort(), outNodeId(), outPort()
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
                              .arg(m_outNodeId).arg(m_outPort)
                              .arg(m_inNodeId).arg(m_inPort));
}
