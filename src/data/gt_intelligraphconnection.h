/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLIGRAPHCONNECTION_H
#define GT_INTELLIGRAPHCONNECTION_H

#include "gt_igglobals.h"

#include "gt_object.h"
#include "gt_intproperty.h"

#include <QtNodes/Definitions>

class GtIntelliGraphConnection : public GtObject
{
    Q_OBJECT

    using QtConnectionId = QtNodes::ConnectionId;
    using NodeId    = gt::ig::NodeId;
    using PortIndex = gt::ig::PortIndex;

public:

    Q_INVOKABLE GtIntelliGraphConnection(GtObject* parent = nullptr);
    GtIntelliGraphConnection(QtConnectionId conId, GtObject* parent = nullptr) :
        GtIntelliGraphConnection(parent)
    {
        fromConnectionId(conId);
    }

    QtConnectionId toConnectionId() const;

    bool fromConnectionId(QtConnectionId connection);

    bool fromJson(QJsonObject const& json);

    QJsonObject toJson() const;

    NodeId inNodeId() const { return NodeId{gt::ig::fromInt(m_inNodeId)}; }
    void setInNodeId(NodeId nodeId) { m_inNodeId = nodeId; }
    
    PortIndex inPortIdx() const { return PortIndex{gt::ig::fromInt(m_inPortIdx)}; }
    void setInPortIdx(PortIndex port) { m_inPortIdx = port; }

    NodeId outNodeId() const { return NodeId{gt::ig::fromInt(m_outNodeId)}; }
    void setOutNodeId(NodeId nodeId) { m_outNodeId = nodeId; }
    
    PortIndex outPortIdx() const { return PortIndex{gt::ig::fromInt(m_outPortIdx)}; }
    void setOutPortIdx(PortIndex port) { m_outPortIdx = port; }

    bool isValid() const;

private:

    /// node id IN (should be unsigned)
    GtIntProperty m_inNodeId;
    /// port idx IN (should be unsigned)
    GtIntProperty m_inPortIdx;
    /// node id IN (should be unsigned)
    GtIntProperty m_outNodeId;
    /// port idx IN (should be unsigned)
    GtIntProperty m_outPortIdx;

    void updateObjectName();
};

#endif // GT_INTELLIGRAPHCONNECTION_H
