/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GUIDATAUI_H
#define GT_INTELLI_GUIDATAUI_H

#include <gt_objectui.h>

namespace intelli
{

class GuiDataUI : public GtObjectUI
{
    Q_OBJECT

public:

    Q_INVOKABLE GuiDataUI();

    QIcon icon(GtObject* obj) const override;
};

} // namespace intelli

#endif // GT_INTELLI_GUIDATAUI_H
