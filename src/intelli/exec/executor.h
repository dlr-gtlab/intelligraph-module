/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 24.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_EXECUTOR_H
#define GT_INTELLI_EXECUTOR_H

#include "intelli/globals.h"

#include <QPointer>

namespace intelli
{

struct NodeImpl;
class Node;
class NodeData;
class GraphExecutionModel;

class Executor : public QObject
{
    Q_OBJECT

public:

    using NodeDataPtr  = std::shared_ptr<const NodeData>;

    using PortIndex = intelli::PortIndex;

    Executor();

    virtual bool evaluateNode(Node& node, GraphExecutionModel& model, PortIndex idx = PortIndex{}) = 0;

    virtual bool isReady() const;

protected:
    
    static NodeDataPtr doEvaluate(Node& node, PortIndex idx);
    static NodeDataPtr doEvaluate(Node& node);

    static GraphExecutionModel* accessExecModel(Node& node);
};

} // namespace intelli

#endif // GT_INTELLI_EXECUTOR_H
