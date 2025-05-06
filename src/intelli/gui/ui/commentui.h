/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_COMMENTUI_H
#define GT_INTELLI_COMMENTUI_H

#include <gt_objectui.h>

namespace intelli
{

class CommentUI : public GtObjectUI
{
    Q_OBJECT

public:

    Q_INVOKABLE CommentUI();

    QIcon icon(GtObject* obj) const override;
};

} // namespace intelli;

#endif // GT_INTELLI_COMMENTUI_H
