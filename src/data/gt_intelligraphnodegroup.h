/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLIGRAPHNODEGROUP_H
#define GT_INTELLIGRAPHNODEGROUP_H

#include "gt_intelligraphnode.h"

class GtIntelliGraph;
class GtIgGroupInputProvider;
class GtIgGroupOutputProvider;

class GtIntelliGraphNodeGroup : public GtIntelliGraphNode
{
    Q_OBJECT

    friend class GtIgGroupInputProvider;

public:

    Q_INVOKABLE GtIntelliGraphNodeGroup();

    GtIntelliGraph* graph();
    GtIntelliGraph const* graph() const;

    GtIgGroupInputProvider* inputProvider();
    GtIgGroupInputProvider const* inputProvider() const;

    GtIgGroupOutputProvider* outputProvider();
    GtIgGroupOutputProvider const* outputProvider() const;

    bool setOutData(PortIndex idx, NodeData data);

protected:

    NodeData eval(PortId outId) override;

private:

    std::vector<NodeData> m_outData;
};

#endif // GT_INTELLIGRAPHNODEGROUP_H
