/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#ifndef GT_INTELLI_CONDITIONALGROUPNODE_H
#define GT_INTELLI_CONDITIONALGROUPNODE_H

#include <intelli/graph.h>
#include <intelli/node/groupinputprovider.h>
#include <intelli/node/groupoutputprovider.h>

namespace intelli
{
class ConditionalInputProvider;
class ConditionalOutputProvider;

class ConditionalGroupNode : public Graph
{
    Q_OBJECT

public:

    enum BranchType
    {
        IfBranch,
        ElseBranch
    };

    Q_INVOKABLE ConditionalGroupNode();

    ConditionalInputProvider* inputProvider(BranchType type);
    ConditionalInputProvider const* inputProvider(BranchType type) const;

    ConditionalOutputProvider* outputProvider(BranchType type);
    ConditionalOutputProvider const* outputProvider(BranchType type) const;

    PortId addDataInPort(PortInfo info);

    PortId addDataOutPort(PortInfo info);

    /**
     * @brief initializes the input and output of this graph
     */
    void initInputOutputProviders() final;

protected:

    using Graph::inputProvider;

private:

    PortId m_condition{};

    struct Impl;

private slots:

    void onInPortInserted(PortType actualType, PortIndex idx);

    void onInPortChanged(PortId portId);

    void onInPortDeleted(PortType actualType, PortIndex idx);

    void onOutPortInserted(PortType actualType, PortIndex idx);

    void onOutPortChanged(PortId portId);

    void onOutPortDeleted(PortType actualType, PortIndex idx);
};

class ConditionalInputProvider : public GroupInputProvider
{
    Q_OBJECT

public:

    Q_INVOKABLE ConditionalInputProvider(ConditionalGroupNode::BranchType type = ConditionalGroupNode::IfBranch);

protected:

    void eval() override {}
};

class ConditionalOutputProvider : public GroupOutputProvider
{
    Q_OBJECT

public:

    Q_INVOKABLE ConditionalOutputProvider(ConditionalGroupNode::BranchType type = ConditionalGroupNode::IfBranch);

protected:

    void eval() override {}
};

} // namespace intelli

#endif // GT_INTELLI_CONDITIONALGROUPNODE_H
