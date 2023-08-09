/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_CONNECTION_H
#define GT_INTELLI_CONNECTION_H

#include "intelli/globals.h"

#include "gt_object.h"
#include "gt_intproperty.h"

namespace QtNodes { struct ConnectionId; }

namespace intelli
{

class Connection : public GtObject
{
    Q_OBJECT

    using ConnectionId = QtNodes::ConnectionId;

public:

    Q_INVOKABLE Connection(GtObject* parent = nullptr);

    Connection(ConnectionId conId, GtObject* parent = nullptr);

    ConnectionId connectionId() const;

    bool fromConnectionId(ConnectionId connection);

    NodeId inNodeId() const;
    void setInNodeId(NodeId nodeId) { m_inNodeId = nodeId; }
    
    PortIndex inPortIdx() const;
    void setInPortIdx(PortIndex port) { m_inPortIdx = port; }

    NodeId outNodeId() const;
    void setOutNodeId(NodeId nodeId) { m_outNodeId = nodeId; }
    
    PortIndex outPortIdx() const;
    void setOutPortIdx(PortIndex port) { m_outPortIdx = port; }

    bool isValid() const;

    void updateObjectName();

private:

    /// node id IN (should be unsigned)
    GtIntProperty m_inNodeId;
    /// port idx IN (should be unsigned)
    GtIntProperty m_inPortIdx;
    /// node id IN (should be unsigned)
    GtIntProperty m_outNodeId;
    /// port idx IN (should be unsigned)
    GtIntProperty m_outPortIdx;
};

} // namespace intelli;

#endif // GT_INTELLI_CONNECTION_H
