/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 24.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLIGRAPHEXECUTOR_H
#define GT_INTELLIGRAPHEXECUTOR_H

#include "gt_igglobals.h"

struct GtIntelliGraphNodeImpl;
class GtIntelliGraphNode;
class GtIgNodeData;

class GtIntellIGraphExecutor : public QObject
{
    Q_OBJECT

public:

    using PortIndex = gt::ig::PortIndex;

    GtIntellIGraphExecutor();

    virtual bool evaluateNode(GtIntelliGraphNode& node) = 0;

    virtual bool evaluatePort(GtIntelliGraphNode& node, PortIndex idx) = 0;

    virtual bool isReady() const;

protected:

    virtual bool canEvaluateNode(GtIntelliGraphNode& node, PortIndex outIdx = PortIndex{});

    bool doEvaluate(GtIntelliGraphNode& node, PortIndex idx);

    void doEvaluateAndDiscard(GtIntelliGraphNode& node);

    GtIntelliGraphNodeImpl& accessImpl(GtIntelliGraphNode& node);
};

#endif // GTINTELLIGRAPHEXECUTOR_H
