/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_BINARYDISPLAY_H
#define GT_INTELLI_BINARYDISPLAY_H

#include <intelli/dynamicnode.h>

namespace intelli
{

class BinaryDisplayNode : public DynamicNode
{
    Q_OBJECT

public:

    Q_INVOKABLE BinaryDisplayNode();

    unsigned inputValue();
};

} // namespace intelli

#endif // GT_INTELLI_BINARYDISPLAY_H
