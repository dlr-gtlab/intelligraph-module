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
#include "gt_intelligraph_exports.h"

#include "gt_object.h"
#include "gt_intproperty.h"

#include <QtNodes/Definitions>

class GtIntelliGraphConnection : public GtObject
{
    Q_OBJECT

    using ConnectionId = QtNodes::ConnectionId;

public:

    Q_INVOKABLE GtIntelliGraphConnection(GtObject* parent = nullptr);

    ConnectionId toConnectionId() const
    {
        return ConnectionId{
            outNodeId(), outPortIdx(), inNodeId(), inPortIdx()
        };
    }

    bool fromConnectionId(ConnectionId connection);

    bool fromJson(QJsonObject const& json);

    QJsonObject toJson() const;

    gt::ig::NodeId inNodeId() const { return gt::ig::fromInt(m_inNodeId); }
    
    gt::ig::PortIndex inPortIdx() const { return gt::ig::fromInt(m_inPortIdx); }

    gt::ig::NodeId outNodeId() const { return gt::ig::fromInt(m_outNodeId); }
    
    gt::ig::PortIndex outPortIdx() const { return gt::ig::fromInt(m_outPortIdx); }

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
