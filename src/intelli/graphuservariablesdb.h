/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHUSERVARIABLESDB_H
#define GT_INTELLI_GRAPHUSERVARIABLESDB_H

#include <gt_object.h>

namespace intelli
{

class GraphUserVariablesDB : public GtObject
{
    Q_OBJECT

public:

    Q_INVOKABLE GraphUserVariablesDB();
};

}

#endif // GT_INTELLI_GRAPHUSERVARIABLESDB_H
