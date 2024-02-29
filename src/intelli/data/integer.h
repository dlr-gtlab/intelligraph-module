/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 28.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  E-Mail: jens.schmeink@dlr.de
 */

#ifndef GT_INTELLI_INTEGERDATA_H
#define GT_INTELLI_INTEGERDATA_H

#include "intelli/nodedata.h"

namespace intelli
{

class GT_INTELLI_EXPORT IntegerData : public NodeData
{
    Q_OBJECT

public:

    Q_INVOKABLE IntegerData(int val = {});

    Q_INVOKABLE int value() const;

private:
    int m_data;
};

} // namespace intelli

#endif // GT_INTELLI_INTEGERDATA_H
