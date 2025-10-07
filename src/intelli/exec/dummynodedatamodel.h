/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_DUMMYNODEDATAMODEL_H
#define GT_INTELLI_DUMMYNODEDATAMODEL_H

#include <intelli/nodedatainterface.h>
#include <intelli/graphdatamodel.h>
#include <intelli/graphuservariables.h>

#include <QPointer>

namespace intelli
{

class Node;

/**
 * @brief The DummyDataModel class.
 * Helper class to set and access data of a single node
 */
class DummyNodeDataModel : public NodeDataInterface
{
public:

    explicit DummyNodeDataModel(Node& node);

    NodeDataPtrList nodeData(PortType type) const;

    NodeDataSet nodeData(NodeUuid const& nodeUuid, PortId portId) const override;

    NodeDataPtrList nodeData(NodeUuid const& nodeUuid, PortType type) const override;

    bool setNodeData(PortId portId, NodeDataSet data);

    bool setNodeData(PortType type, NodeDataPtrList const& data);

    bool setNodeData(NodeUuid const& nodeUuid, PortId portId, NodeDataSet data) override;

    bool setNodeData(NodeUuid const& nodeUuid, PortType type, NodeDataPtrList const& data) override;

    NodeEvalState nodeEvalState(NodeUuid const& nodeUuid) const override;

    bool evaluationSuccessful() const { return m_success; }

    void setNodeEvaluationFailed(NodeUuid const& nodeUuid) override;

    GraphUserVariables const* userVariables() const override { return m_userVariables; }

    void setUserVariables(GraphUserVariables const* userVariables) { m_userVariables = userVariables; }

    GtObject* scope() override { return m_scope; }

    void setScope(GtObject* scope) { m_scope = scope; }

private:

    Node* m_node = nullptr;
    data_model::DataItem m_data;
    QPointer<GtObject> m_scope;
    QPointer<GraphUserVariables const> m_userVariables;
    bool m_success = true;
};

} // namespace intelli

#endif // GT_INTELLI_DUMMYNODEDATAMODEL_H
