/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_IGGROUPOUTPUTPROVIDER_H
#define GT_IGGROUPOUTPUTPROVIDER_H

#include "gt_intelligraphnode.h"

class GtIgGroupOutputProvider : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgGroupOutputProvider();

    unsigned nPorts(PortType type) const override;

    NodeDataType dataType(PortType type, PortIndex idx) const override;

    NodeData outData(PortIndex const port) override;

    void setInData(NodeData data, PortIndex const port) override;

private:

    std::vector<NodeData> m_data;

};

#endif // GT_IGGROUPOUTPUTPROVIDER_H
