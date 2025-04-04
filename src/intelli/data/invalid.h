/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_INVALIDDATA_H
#define GT_INTELLI_INVALIDDATA_H

#include <intelli/nodedata.h>

namespace intelli
{

/**
 * @brief The InvalidData class. Invalid node data. Always yields null.
 */
class GT_INTELLI_EXPORT InvalidData : public intelli::NodeData
{
    Q_OBJECT

public:

    Q_INVOKABLE InvalidData();
};

} // namespace intelli

#endif // GT_INTELLI_INVALIDDATA_H
