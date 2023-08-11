/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_NUMBERDISPLAYNODE_H
#define GT_INTELLI_NUMBERDISPLAYNODE_H

#include <intelli/node.h>

namespace intelli
{

class NumberDisplayNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE NumberDisplayNode();
};

} // namespace intelli

#endif // GT_INTELLI_NUMBERDISPLAYNODE_H
