/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_iggroupproviderui.h"

#include "gt_iggroupinputprovider.h"
#include "gt_iggroupoutputprovider.h"

#include "gt_icons.h"

GtIgGroupProviderUI::GtIgGroupProviderUI()
{
    addPortAction(tr("Delete Port"), deleteInputProviderPort)
        .setIcon(gt::gui::icon::delete_())
        .setVisibilityMethod(isInputProvider);
    addPortAction(tr("Delete Port"), deleteOutputProviderPort)
        .setIcon(gt::gui::icon::delete_())
        .setVisibilityMethod(isOutputProvider);
}

void
GtIgGroupProviderUI::deleteInputProviderPort(GtIntelliGraphNode* node, PortType type, PortIndex idx)
{
    auto* provider = qobject_cast<GtIgGroupInputProvider*>(node);
    if (!provider || type != provider->INVERSE_TYPE()) return;

    provider->removePort(idx);
}

void
GtIgGroupProviderUI::deleteOutputProviderPort(GtIntelliGraphNode* node, PortType type, PortIndex idx)
{
    auto* provider = qobject_cast<GtIgGroupOutputProvider*>(node);
    if (!provider || type != provider->INVERSE_TYPE()) return;

    provider->removePort(idx);
}

bool
GtIgGroupProviderUI::isInputProvider(GtIntelliGraphNode* node, PortType, PortIndex)
{
    return qobject_cast<GtIgGroupInputProvider*>(node);
}

bool
GtIgGroupProviderUI::isOutputProvider(GtIntelliGraphNode* node, PortType, PortIndex)
{
    return qobject_cast<GtIgGroupOutputProvider*>(node);
}

