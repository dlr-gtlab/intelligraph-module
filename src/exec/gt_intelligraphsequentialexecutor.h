/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTINTELLIGRAPHSEQUENTIALEXECUTOR_H
#define GTINTELLIGRAPHSEQUENTIALEXECUTOR_H

#include "gt_intelligraphexecutor.h"

class GtIntelliGraphSequentialExecutor : public GtIntellIGraphExecutor
{
    Q_OBJECT

public:

    bool evaluateNode(GtIntelliGraphNode& node) override;

    bool evaluatePort(GtIntelliGraphNode& node, PortIndex idx) override;

protected:

    bool canEvaluateNode(GtIntelliGraphNode& node, PortIndex outIdx = PortIndex{}) override;

private:

    bool m_evaluating = false;
};

#endif // GTINTELLIGRAPHSEQUENTIALEXECUTOR_H
