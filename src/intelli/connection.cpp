/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/connection.h"

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

    return connectionId().isValid();
}

void
Connection::updateObjectName()
{
    return gt::setUniqueName(
        *this, !connectionId().isValid() ?
                   QStringLiteral("NodeConnection[N/A]") :
                   QStringLiteral("NodeConnection[%1:%2/%3:%4]")
                       .arg(m_outNodeId).arg(m_outPort)
                       .arg(m_inNodeId).arg(m_inPort));
}
