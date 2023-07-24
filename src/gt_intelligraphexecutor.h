/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 24.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTINTELLIGRAPHEXECUTOR_H
#define GTINTELLIGRAPHEXECUTOR_H

#include "gt_igglobals.h"

class GtIntelliGraphNode;

class GtIntellIGraphExecutor
{
public:

    using PortIndex    = gt::ig::PortIndex;

    GtIntellIGraphExecutor();

    void evaluateNode(GtIntelliGraphNode& node);

    void evaluatePort(GtIntelliGraphNode& node, PortIndex idx);

private:

    bool canEvaluateNode(GtIntelliGraphNode& node, PortIndex outIdx = PortIndex{});
};

#endif // GTINTELLIGRAPHEXECUTOR_H
