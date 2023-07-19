/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 19.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphconnectionui.h"
#include "gt_intelligraphconnection.h"
#include "gt_intelligraphconnectiongroup.h"

#include "gt_icons.h"

GtIntelliGraphConnectionUI::GtIntelliGraphConnectionUI() = default;

QIcon
GtIntelliGraphConnectionUI::icon(GtObject* obj) const
{
    if (qobject_cast<GtIntelliGraphConnection*>(obj))
    {
        return gt::gui::icon::vectorBezier2();
    }
    if (qobject_cast<GtIntellIGraphConnectionGroup*>(obj))
    {
        return gt::gui::icon::data();
    }
    return {};
}

