/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_STRINGLISTINPUTNODE_H
#define GT_INTELLI_STRINGLISTINPUTNODE_H

#include <intelli/node.h>

#include <gt_propertystructcontainer.h>

#include <QTextEdit>

namespace intelli
{

class StringListInputNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE StringListInputNode();

protected:

    NodeDataPtr eval(PortId outId) override;

private:

    GtPropertyStructContainer m_values;

    QStringList values() const;
};

} // namespace intelli

#endif // GT_INTELLI_STRINGLISTINPUTNODE_H
