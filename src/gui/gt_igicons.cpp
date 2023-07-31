/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 31.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_igicons.h"

QIcon const&
gt::gui::icon::ig::intelliGraph()
{
    static QIcon icon = gt::gui::getIcon(
        QStringLiteral(":/intelligraph-icons/intelligraph.svg")
    );
    return icon;
}

QIcon const&
gt::gui::icon::ig::node()
{
    static QIcon icon = gt::gui::getIcon(
        QStringLiteral(":/intelligraph-icons/node.svg")
    );
    return icon;
}
