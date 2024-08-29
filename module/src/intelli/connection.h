/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_CONNECTION_H
#define GT_INTELLI_CONNECTION_H

#include <intelli/exports.h>
#include <intelli/globals.h>
#include <intelli/property/uint.h>

#include <gt_object.h>
#include <gt_intproperty.h>

namespace intelli
{

/**
 * @brief The Connection class.
 * Represents a connection between two nodes and their resprective output and
 * input ports
 */
class GT_INTELLI_EXPORT Connection : public GtObject
{
    Q_OBJECT

public:

    Q_INVOKABLE Connection(GtObject* parent = nullptr);

    Connection(ConnectionId conId, GtObject* parent = nullptr);

    ConnectionId connectionId() const;

    bool fromConnectionId(ConnectionId connection);

    NodeId inNodeId() const { return NodeId{m_inNodeId}; }
    void setInNodeId(NodeId nodeId) { m_inNodeId = nodeId; }
    
    PortId inPort() const { return PortId{m_inPort}; }
    void setInPort(PortId port) { m_inPort = port; }

    NodeId outNodeId() const { return NodeId{m_outNodeId}; }
    void setOutNodeId(NodeId nodeId) { m_outNodeId = nodeId; }
    
    PortId outPort() const { return PortId{m_outPort}; }
    void setOutPort(PortId port) { m_outPort = port; }

    void updateObjectName();

private:

    /// node id IN (should be unsigned)
    UIntProperty m_inNodeId;
    /// port IN (should be unsigned)
    UIntProperty m_inPort;
    /// node id IN (should be unsigned)
    UIntProperty m_outNodeId;
    /// port IN (should be unsigned)
    UIntProperty m_outPort;
};

} // namespace intelli;

#endif // GT_INTELLI_CONNECTION_H
