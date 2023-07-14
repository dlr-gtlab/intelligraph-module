/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 14.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTINTELLIGRAPHNODEUI_H
#define GTINTELLIGRAPHNODEUI_H

#include "gt_objectui.h"
#include "gt_intelligraph_exports.h"

class GT_IG_EXPORT GtIntelliGraphNodeUI : public GtObjectUI
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIntelliGraphNodeUI();

    QIcon icon(GtObject* obj) const override;

    static void addNodeGraph(GtObject* obj);

    static void renameNode(GtObject* obj);

    static void clearNodeGraph(GtObject* obj);

    static void loadNodeGraph(GtObject* obj);

    static bool isGraphObject(GtObject* obj);

    static bool isNodeObject(GtObject* obj);

    static bool canRenameNodeObject(GtObject* obj);

    QStringList openWith(GtObject* obj) override;
};

#endif // GTINTELLIGRAPHNODEUI_H
