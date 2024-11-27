/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/ui/connectionui.h"
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

