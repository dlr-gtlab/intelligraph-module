/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTIGCONDITIONALNODE_H
#define GTIGCONDITIONALNODE_H

#include "gt_intelligraphnode.h"

#include "gt_igstringselectionproperty.h"

class GtIgConditionalNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgConditionalNode();

protected:

    NodeData eval(PortId outId) override;

private:

    GtIgStringSelectionProperty m_dataType;

    PortId m_inCondition, m_inData, m_outIf, m_outElse;
};

#endif // GTIGCONDITIONALNODE_H
