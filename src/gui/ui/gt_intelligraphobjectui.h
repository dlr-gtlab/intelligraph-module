/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GTINTELLIGRAPHOBJECTUI_H
#define GTINTELLIGRAPHOBJECTUI_H

#include "gt_objectui.h"

class GtIntelliGraphObjectUI : public GtObjectUI
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIntelliGraphObjectUI();

    QIcon icon(GtObject* obj) const override;

    static void addNodeCategory(GtObject* obj);

    static void addNodeGraph(GtObject* obj);

    static void clearNodeGraph(GtObject* obj);

    static void loadNodeGraph(GtObject* obj);

    static bool isCategoryObject(GtObject* obj);

    static bool isPackageObject(GtObject* obj);

    static bool isGraphObject(GtObject* obj);

    QStringList openWith(GtObject* obj) override;
};

#endif // GTINTELLIGRAPHOBJECTUI_H
