/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHCATEGORYUI_H
#define GT_INTELLI_GRAPHCATEGORYUI_H

#include <gt_objectui.h>

namespace intelli
{

class GraphCategoryUI : public GtObjectUI
{
    Q_OBJECT

public:
    
    Q_INVOKABLE GraphCategoryUI();

    QIcon icon(GtObject* obj) const override;

    static void addNodeGraph(GtObject* obj);

    static bool isCategoryObject(GtObject* obj);
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHCATEGORYUI_H
