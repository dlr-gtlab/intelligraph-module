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

enum class ConditionalBranchType : unsigned
{
    IfBranch = 0,
    ElseBranch
};

class ConditionalInputProvider : public GraphInputProvider
{
    Q_OBJECT
    ConditionalBranchType m_type;

public:

    Q_INVOKABLE ConditionalInputProvider(ConditionalBranchType type = ConditionalBranchType::IfBranch) :
        m_type(type)
    {}

    ConditionalBranchType branchType() const { return m_type; }
};

class ConditionalOutputProvider : public GraphOutputProvider
{
    Q_OBJECT
    ConditionalBranchType m_type;

public:

    Q_INVOKABLE ConditionalOutputProvider(ConditionalBranchType type = ConditionalBranchType::IfBranch) :
        m_type(type)
    {}

    ConditionalBranchType branchType() const { return m_type; }
};

class GT_INTELLI_EXPORT ConditionalGroupNode : public Graph
{
    Q_OBJECT

public:

    Q_INVOKABLE ConditionalGroupNode();

    ConditionalInputProvider* inputProvider(ConditionalBranchType type);
    ConditionalInputProvider const* inputProvider(ConditionalBranchType type) const;

    ConditionalOutputProvider* outputProvider(ConditionalBranchType type);
    ConditionalOutputProvider const* outputProvider(ConditionalBranchType type) const;

    PortId addDataInPort(PortInfo info);

    PortId addDataOutPort(PortInfo info);

    bool isDataPort(PortId portId) const;

    void updateDataPort(PortId portId, PortInfo newPort);

    void removeDataPort(PortId portId);

    /**
     * @brief initializes the input and output of this graph
     */
    void initInputOutputProviders() final;

protected:

    using Graph::inputProvider;
    using Graph::inputNode;
    using Graph::outputProvider;
    using Graph::outputNode;

    void eval() override;

    void onObjectDataMerged() override;

private:

    PortId m_condition{};

    struct Impl;

private slots:

    void onPortInserted(PortType type, PortIndex idx);

    void onPortChanged(PortId portId);

    void onPortDeleted(PortType type, PortIndex idx);
};

} // namespace intelli

#endif // GT_INTELLI_CONDITIONALGROUPNODE_H
