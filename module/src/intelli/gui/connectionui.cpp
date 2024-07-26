/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 19.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/gui/connectionui.h"
#include "intelli/connection.h"
#include "intelli/connectiongroup.h"

#include "gt_icons.h"

using namespace intelli;

ConnectionUI::ConnectionUI() = default;

QIcon
ConnectionUI::icon(GtObject* obj) const
{
    if (qobject_cast<Connection*>(obj))
    {
        return gt::gui::icon::vectorBezier2();
    }
    if (qobject_cast<ConnectionGroup*>(obj))
    {
        return gt::gui::icon::data();
    }
    return {};
}

