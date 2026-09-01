/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_LIST_H
#define GT_INTELLI_LIST_H

#include <intelli/nodedata.h>

namespace intelli
{

class GT_INTELLI_EXPORT ListData : public NodeData
{
    Q_OBJECT

public:
    
    Q_INVOKABLE ListData();
};

} // namespace intelli

#endif // GT_INTELLI_LIST_H
