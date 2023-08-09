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


namespace intelli
{

struct NodeImpl;
class Node;
class NodeData;

class Executor : public QObject
{
    Q_OBJECT

public:

    using PortIndex = intelli::PortIndex;

    Executor();
    
    virtual bool evaluateNode(Node& node) = 0;
    
    virtual bool evaluatePort(Node& node, PortIndex idx) = 0;

    virtual bool isReady() const;

protected:
    
    virtual bool canEvaluateNode(Node& node, PortIndex outIdx = PortIndex{});
    
    static bool doEvaluate(Node& node, PortIndex idx);
    
    static void doEvaluateAndDiscard(Node& node);
    
    static NodeImpl& accessImpl(Node& node);
};

} // namespace intelli

#endif // GT_INTELLI_EXECUTOR_H
