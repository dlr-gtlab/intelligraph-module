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

#if GT_VERSION >= 0x020100
    /**
     * @brief hasValidationRegExp
     * @param obj - the underlying object to evaluate
     * @return true, because element has validator
     */
    bool hasValidationRegExp(GtObject* obj) override;

    /**
     * @brief validatorRegExp
     * @param obj - the underlying object to evaluate
     * @return Regexp to accept letters, digits, -, _
     */
    QRegExp validatorRegExp(GtObject* obj) override;
#endif
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHCATEGORYUI_H
