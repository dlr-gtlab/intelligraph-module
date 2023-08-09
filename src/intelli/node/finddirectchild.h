/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 5.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_FINDDIRECTCHILDNODE_H
#define GT_INTELLI_FINDDIRECTCHILDNODE_H

#include <intelli/node.h>

#include <gt_stringproperty.h>

namespace intelli
{

class FindDirectChildNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE FindDirectChildNode();

protected:

    NodeDataPtr eval(PortId outId) override;

private:

    GtStringProperty m_childClassName;

    PortId m_in, m_out;
};

} // namespace intelli

#endif // GT_INTELLI_FINDDIRECTCHILDNODE_H
