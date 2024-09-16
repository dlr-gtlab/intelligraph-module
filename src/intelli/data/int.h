/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_INTDATA_H
#define GT_INTELLI_INTDATA_H

#include <intelli/nodedata.h>

namespace intelli
{

class GT_INTELLI_EXPORT IntData : public NodeData
{
    Q_OBJECT

public:

    Q_INVOKABLE IntData(int val = {});

    Q_INVOKABLE int value() const;

private:
    int m_data;
};

} // namespace intelli

#endif // GT_INTELLI_INTDATA_H
