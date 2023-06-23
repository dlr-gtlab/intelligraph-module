/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_IGSTRINGLISTINPUTNODE_H
#define GT_IGSTRINGLISTINPUTNODE_H

#include "gt_intelligraphnode.h"
#include "gt_igvolatileptr.h"

#include "gt_propertystructcontainer.h"

#include <QTextEdit>

class GtIgStringListData2;
class GtIgStringListInputNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgStringListInputNode();

protected:

    NodeData eval(PortId outId) override;

private:

    GtPropertyStructContainer m_values;

    QStringList values() const;
};

#endif // GT_IGSTRINGLISTINPUTNODE_H
