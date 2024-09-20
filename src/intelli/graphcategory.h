/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHCATEGORY_H
#define GT_INTELLI_GRAPHCATEGORY_H

#include <intelli/exports.h>

#include <gt_object.h>

namespace intelli
{

class GraphCategory : public GtObject
{
    Q_OBJECT

public:

    Q_INVOKABLE GraphCategory();
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHCATEGORY_H
