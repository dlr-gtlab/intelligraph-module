/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLIGRAPHPACKAGEUI_H
#define GT_INTELLIGRAPHPACKAGEUI_H

#include "gt_objectui.h"

class GtIntelliGraphPackageUI : public GtObjectUI
{
    Q_OBJECT

public:
    
    Q_INVOKABLE GtIntelliGraphPackageUI();

    QIcon icon(GtObject* obj) const override;

    static void addNodeCategory(GtObject* obj);

    static void addNodeGraph(GtObject* obj);

    static bool isCategoryObject(GtObject* obj);

    static bool isPackageObject(GtObject* obj);
};

#endif // GT_INTELLIGRAPHPACKAGEUI_H
