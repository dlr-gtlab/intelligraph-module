/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_PACKAGEUI_H
#define GT_INTELLI_PACKAGEUI_H

#include <gt_objectui.h>

namespace intelli
{

class PackageUI : public GtObjectUI
{
    Q_OBJECT

public:
    
    Q_INVOKABLE PackageUI();

    QIcon icon(GtObject* obj) const override;

    static void addNodeCategory(GtObject* obj);

    static void addNodeGraph(GtObject* obj);

    static bool isCategoryObject(GtObject* obj);

    static bool isPackageObject(GtObject* obj);
};

} // namespace intelli

#endif // GT_INTELLI_PACKAGEUI_H
