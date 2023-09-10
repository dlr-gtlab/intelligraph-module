/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_CONNECTION_H
#define GT_INTELLI_CONNECTION_H

#include "intelli/exports.h"
#include "intelli/globals.h"

#include "gt_object.h"
#include "gt_intproperty.h"

namespace intelli
{

/**
 * @brief The Connection class.
 * Represents a connection between two nodes and their resprective output and
 * input ports
 */
class GT_INTELLI_TEST_EXPORT Connection : public GtObject
{
    Q_OBJECT

public:

    Q_INVOKABLE Connection(GtObject* parent = nullptr);

    Connection(ConnectionId conId, GtObject* parent = nullptr);

    ConnectionId connectionId() const;

    bool fromConnectionId(ConnectionId connection);

    NodeId inNodeId() const;
    void setInNodeId(NodeId nodeId) { m_inNodeId = nodeId; }
    
    PortId inPort() const;
    void setInPort(PortId port) { m_inPort = port; }

    NodeId outNodeId() const;
    void setOutNodeId(NodeId nodeId) { m_outNodeId = nodeId; }
    
    PortId outPort() const;
    void setOutPort(PortId port) { m_outPort = port; }

    bool isValid() const;

    void updateObjectName();

private:

    /// node id IN (should be unsigned)
    GtIntProperty m_inNodeId;
    /// port IN (should be unsigned)
    GtIntProperty m_inPort;
    /// node id IN (should be unsigned)
    GtIntProperty m_outNodeId;
    /// port IN (should be unsigned)
    GtIntProperty m_outPort;
};

} // namespace intelli;

#endif // GT_INTELLI_CONNECTION_H
