/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 * 
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email: 
 */

/*
 * generated 1.2.0
 */
 
#include "gt_mdilauncher.h"

#include "gt_igprojectui.h"

GtIgProjectUI::GtIgProjectUI()
{
    setObjectName("ProjectUI");

//    addSingleAction("Open Node Editor", "openNodeEditor");
}

void
GtIgProjectUI::openNodeEditor(GtObject* /*obj*/)
{
    gtMdiLauncher->open("GtIntelliGraphEditor");
}

