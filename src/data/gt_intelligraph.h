/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLIGRAPH_H
#define GT_INTELLIGRAPH_H

#include "gt_intelligraphnode.h"

#include <QtNodes/Definitions>

#include <QPointer>

class GtObjectGroup;
class GtIgGroupInputProvider;
class GtIgGroupOutputProvider;
class GtIntelliGraphConnection;
class GtIntellIGraphConnectionGroup;
class GtIntelliGraphModelAdapter;

namespace QtNodes { struct ConnectionId; }

class GtIntelliGraph : public GtIntelliGraphNode
{
    Q_OBJECT

    friend class GtIgGroupInputProvider;
    friend class GtIgGroupOutputProvider;

    template <gt::ig::PortType>
    friend class GtIgAbstractGroupProvider;

    using ConnectionId = QtNodes::ConnectionId;

public:

    Q_INVOKABLE GtIntelliGraph();
    ~GtIntelliGraph();

    QList<GtIntelliGraphNode*> nodes();
    QList<GtIntelliGraphNode const*> nodes() const;

    QList<GtIntelliGraphConnection*> connections();
    QList<GtIntelliGraphConnection const*> connections() const;

    /**
     * @brief Returns the group object in which all connections are stored
     * (should never be null)
     * @return Object group
     */
    GtIntellIGraphConnectionGroup& connectionGroup();
    GtIntellIGraphConnectionGroup const& connectionGroup() const;

    GtIntelliGraphNode* findNode(NodeId nodeId);
    GtIntelliGraphNode const* findNode(NodeId nodeId) const;

    GtIntelliGraphConnection* findConnection(ConnectionId const& conId);
    GtIntelliGraphConnection const* findConnection(ConnectionId const& conId) const;

    QList<GtIntelliGraph*> subGraphs();
    QList<GtIntelliGraph const*> subGraphs() const;

    GtIgGroupInputProvider* inputProvider();
    GtIgGroupInputProvider const* inputProvider() const;

    GtIgGroupOutputProvider* outputProvider();
    GtIgGroupOutputProvider const* outputProvider() const;

    /**
     * @brief Returns the graph model adapter (may be null)
     * @return Graph model
     */
    GtIntelliGraphModelAdapter* findModelAdapter();
    GtIntelliGraphModelAdapter const* findModelAdapter() const;

    void insertOutData(PortIndex idx);

    bool setOutData(PortIndex idx, NodeData data);

    /**
     * @brief Clears all nodes and connections
     */
    void clear();

    /**
     * @brief Appends the node to the intelli graph. Use this function instead
     * of appending the child directly. Node may change its id if its
     * already occupied
     * @param node Node to append
     * @param policy Whether to generate a new id if necessary
     * @return success
     */
    GtIntelliGraphNode* appendNode(std::unique_ptr<GtIntelliGraphNode> node,
                                   gt::ig::NodeIdPolicy policy = gt::ig::UpdateNodeId);

    /**
     * @brief Appends the connection to intelli graph. Use this function instead
     * of appending the child directly. Aborts if the connection already exists
     * @param connection Connection to append
     * @return success
     */
    GtIntelliGraphConnection* appendConnection(std::unique_ptr<GtIntelliGraphConnection> connection);

    QVector<NodeId> appendObjects(std::vector<std::unique_ptr<GtIntelliGraphNode>>& nodes,
                                  std::vector<std::unique_ptr<GtIntelliGraphConnection>>& connections);

    /**
     * @brief Deletes the node specified by node id. Returns true on success.
     * @param nodeId Node to delete
     * @return True if successful else false
     */
    bool deleteNode(NodeId nodeId);

    /**
     * @brief Deletes the connection specified by the connection id.
     * Returns true on success.
     * @param nodeId Node to delete
     * @return True if successful else false
     */
    bool deleteConnection(ConnectionId connectionId);

    /**
     * @brief Updates the position of the node associated with nodeId.
     * @param nodeId Node to update
     * @param pos New position
     */
    void setNodePosition(GtIntelliGraphNode* node, QPointF pos);

    /**
     * @brief Creates a graph model adapter if it does not exists already.
     * It is uesd to evaluate nodes
     * @param policy Inidctaes whether the instance should be considered an
     * active or a dummy model
     * @return Graph model adapter
     */
    GtIntelliGraphModelAdapter* makeModelAdapter(gt::ig::ModelPolicy policy = gt::ig::ActiveModel);

    /**
     * @brief Clears the graph model adapter thus stopping the evaluation of all
     * nodes. Should be called once the graph model is no longer used.
     * @param force Force to close the model regardless of its model policy
     */
    void clearModelAdapter(bool force = true);

    /**
     * @brief initGroupProviders
     */
    void initGroupProviders();

protected:

    // keep graph model up date if a node or connection was restored
    void onObjectDataMerged() override;

    NodeData eval(PortId outId) override;

signals:

    void connectionAppended(GtIntelliGraphConnection* connection);

    void nodeAppended(GtIntelliGraphNode* node);

    void nodePositionChanged(gt::ig::NodeId nodeId, QPointF pos);

private:

    std::vector<NodeData> m_outData;
};

#endif // GT_INTELLIGRAPH_H
