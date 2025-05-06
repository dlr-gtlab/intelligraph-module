/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_COMMENTGROUP_H
#define GT_INTELLI_COMMENTGROUP_H

#include <gt_object.h>

namespace intelli
{

class CommentGroup : public GtObject
{
    Q_OBJECT

public:

    CommentGroup();
};

} // namespace intelli

#endif // GT_INTELLI_COMMENTGROUP_H
