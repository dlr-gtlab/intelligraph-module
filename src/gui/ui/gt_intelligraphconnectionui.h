/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 19.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTINTELLIGRAPHCONNECTIONUI_H
#define GTINTELLIGRAPHCONNECTIONUI_H

#include "gt_objectui.h"


class GtIntelliGraphConnectionUI : public GtObjectUI
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIntelliGraphConnectionUI();

    QIcon icon(GtObject* obj) const override;
};

#endif // GTINTELLIGRAPHCONNECTIONUI_H
