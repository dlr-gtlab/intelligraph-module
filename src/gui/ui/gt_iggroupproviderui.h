/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTIGGROUPPROVIDERUI_H
#define GTIGGROUPPROVIDERUI_H

#include "gt_intelligraphnodeui.h"

class GtIgGroupProviderUI : public GtIntelliGraphNodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgGroupProviderUI();

    static void deleteInputProviderPort(GtIntelliGraphNode* node, PortType type, PortIndex idx);

    static void deleteOutputProviderPort(GtIntelliGraphNode* node, PortType type, PortIndex idx);

    static bool isInputProvider(GtIntelliGraphNode* node, PortType, PortIndex);

    static bool isOutputProvider(GtIntelliGraphNode* node, PortType, PortIndex);
};

#endif // GTIGGROUPPROVIDERUI_H
