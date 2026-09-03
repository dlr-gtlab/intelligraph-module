/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHDATAMODEL_H
#define GT_INTELLI_GRAPHDATAMODEL_H

#include <memory>
#include <intelli/nodedatainterface.h>
#include <intelli/node.h>

namespace intelli
{

class Graph;
class Node;

class GT_INTELLI_EXPORT GraphDataModel : public NodeDataInterface
{
    Q_OBJECT

public:

    GraphDataModel(Graph& graph);
    ~GraphDataModel() override;

    bool isSilent() const;
    void setSilent(bool value);

    void reset();

    GT_NO_DISCARD
    Graph& graph();

    GT_NO_DISCARD
    Graph const& graph() const;

    /**
     * @brief Node-id based overload to access the data of a node.
     * @param graph Graph that is the direct parent of the spcified node
     * @param nodeId Node id
     * @param portId Desired port
     * @return Node dataset (may be null)
     */
    GT_NO_DISCARD
    NodeDataSet nodeData(Graph const& graph, NodeId nodeId, PortId portId) const;
    /**
     * @brief Returns the node data of the given node at the specified port.
     * @param nodeUuid Node's uuid
     * @param portId Desired port
     * @return Node dataset (may be null)
     */
    GT_NO_DISCARD
    NodeDataSet nodeData(NodeUuid const& nodeUuid, PortId portId) const override;
    /**
     * @brief Returns the node data of the given node at the specified port.
     * @param nodeUuid Node's uuid
     * @param type Whether the port is an input or output port
     * @param portIdx Index of the port
     * @return Node dataset (may be null)
     */
    GT_NO_DISCARD
    NodeDataSet nodeData(NodeUuid const& nodeUuid, PortType type, PortIndex portIdx) const;
    /**
     * @brief Returns the node data of the given node as a list. Each entry is
     * associtated with the given port id.
     * @param nodeUuid Node's uuid
     * @param type Whether to access the input or output ports
     * @return List of node datasets (may be null)
     */
    GT_NO_DISCARD
    NodeDataPtrList nodeData(NodeUuid const& nodeUuid, PortType type) const override;

    /**
     * @brief Node-id based overload to set the data of a node.
     * @param graphGraph that is the direct parent of the spcified node
     * @param nodeId Node id
     * @param portId Desired port
     * @param data Data to apply
     * @return success
     */
    bool setNodeData(Graph const& graph, NodeId nodeId, PortId portId, NodeDataSet data);
    /**
     * @brief Sets the node data of the given node at the specified port.
     * @param nodeUuid Node's uuid
     * @param portId Desired port
     * @param data Data to apply
     * @return success
     */
    bool setNodeData(NodeUuid const& nodeUuid, PortId portId, NodeDataSet data) override;
    /**
     * @brief Sets the node data of the given node at the specified port.
     * @param nodeUuid Node's uuid
     * @param type Whether the port is an input or output port
     * @param portIdx Index of the port
     * @param data Data to apply
     * @return success
     */
    bool setNodeData(NodeUuid const& nodeUuid, PortType type, PortIndex portIdx, NodeDataSet data);
    /**
     * @brief Applies a list of node data to the given node.
     * @param nodeUuid Node's uuid
     * @param type Whether to access the input or output ports
     * @param data Data list to apply
     * @return success
     */
    bool setNodeData(NodeUuid const& nodeUuid, PortType type, NodeDataPtrList const& data) override;

    GT_NO_DISCARD
    NodeEvalState nodeEvalState(NodeUuid const& nodeUuid) const override;

    /**
     * @brief Should be called to mark a node as failed.
     * @param nodeUuid Node that failed evaluation
     */
    void setNodeEvaluationFailed(NodeUuid const& nodeUuid) override;

    void setNodeEvaluationOutdated(NodeUuid const& nodeUuid);

    void setNodeEvaluationSuccess(NodeUuid const& nodeUuid);

    void invalidateNode(NodeUuid const& nodeUuid) { setNodeEvaluationOutdated(nodeUuid); }

    /**
     * @brief Returns the user variables object if any exists.
     * @return User variables object (may be null)
     */
    GT_NO_DISCARD
    GraphUserVariables const* userVariables() const override;

    /**
     * @brief Returns the scope object used for evalauation. The scope object
     * is intended to to be used for interfacing with the datamodel.
     * By default the scope object s the current project, but it may be set to
     * a sub datatree or an external tree instead.
     * @return Scope object (may be null).
     */
    GT_NO_DISCARD
    GtObject* scope() override;

    /**
     * @brief Sets the scope object that should be used for evalauation.
     * @param scope Scope object
     */
    void setScope(GtObject& scope);

    void nodeEvaluationStarted(NodeUuid const& nodeUuid) override;

    void nodeEvaluationFinished(NodeUuid const& nodeUuid) override;

signals:

    void evaluationStarted(QString const& nodeUuid);

    void evaluationFinished(QString const& nodeUuid);

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;

    void setupConnections(Graph& graph);

    /// Updates the model if a node was appended
    void onNodeAppended(Node* node);

    /// Updates the model if a node was deleted
    void onNodeDeleted(Graph* graph, NodeId nodeId);

    /// Updates the model if a port was inserted
    void onNodePortInserted(NodeId nodeId, PortType type, PortIndex idx);

    /// Updates the model if a port is about to be deleted
    void onNodePortDeleted(NodeId nodeId, PortType type, PortIndex idx);

    /// Updates the model if a graph was deleted
    void onGraphDeleted();

    void onConnectionAppended(ConnectionUuid con);

    void onConnectionDeleted(ConnectionUuid con);
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHDATAMODEL_H
