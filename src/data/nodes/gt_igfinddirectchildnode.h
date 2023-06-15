/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 5.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_IGFINDDIRECTCHILDNODE_H
#define GT_IGFINDDIRECTCHILDNODE_H

#include "gt_intelligraphnode.h"
#include "gt_igvolatileptr.h"

#include "gt_stringproperty.h"
#include "gt_lineedit.h"

class GtIgObjectData2;
class GtIgFindDirectChildNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgFindDirectChildNode();

protected:

    NodeData eval(PortId outId) override;

private:

    GtStringProperty m_childClassName;

    PortId m_inPort = gt::ig::invalid<PortId>();
};

#endif // GT_IGFINDDIRECTCHILDNODE_H
