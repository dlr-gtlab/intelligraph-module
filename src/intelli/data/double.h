/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_DOUBLEDATA_H
#define GT_INTELLI_DOUBLEDATA_H

#include <intelli/nodedata.h>

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

#endif // GT_INTELLI_DOUBLEDATA_H
