/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHOUTPUTPROVIDER_H
#define GT_INTELLI_GRAPHOUTPUTPROVIDER_H

#include <intelli/node/abstractgroupprovider.h>

namespace intelli
{

class GraphOutputProvider : public AbstractGraphProvider
{
    Q_OBJECT

public:

    Q_INVOKABLE GraphOutputProvider();
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHOUTPUTPROVIDER_H
