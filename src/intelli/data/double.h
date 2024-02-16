/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_DOUBLEDATA_H
#define GT_INTELLI_DOUBLEDATA_H

#include "intelli/nodedata.h"

namespace intelli
{

class GT_INTELLI_EXPORT DoubleData : public NodeData
{
    Q_OBJECT

public:

    Q_INVOKABLE DoubleData(double val = {});

    Q_INVOKABLE double value() const;

private:
    double m_data;
};

} // namespace intelli

using GtIgDoubleData [[deprecated]] = intelli::DoubleData;

#endif // GT_INTELLI_DOUBLEDATA_H
