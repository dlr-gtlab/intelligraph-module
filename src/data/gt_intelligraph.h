/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLIGRAPH_H
#define GT_INTELLIGRAPH_H

#include <QtNodes/Definitions>

#include "gt_intelligraphnode.h"

#include <QPointer>

namespace QtNodes { class DataFlowGraphModel; }

class QJsonObject;

class GtIgGroupInputProvider;
class GtIgGroupOutputProvider;
class GtIntelliGraphConnection;
class GtIntelliGraph : public GtIntelliGraphNode
{
    Q_OBJECT

    friend class GtIgGroupInputProvider;
    friend class GtIgGroupOutputProvider;

    template <gt::ig::PortType>
    friend class GtIgAbstractGroupProvider;

public:

    using DataFlowGraphModel = QtNodes::DataFlowGraphModel;

    using QtNodeId = QtNodes::NodeId;
    using QtConnectionId = QtNodes::ConnectionId;

    Q_INVOKABLE GtIntelliGraph();

    QList<GtIntelliGraphNode*> nodes();
    QList<GtIntelliGraphNode const*> nodes() const;

    QList<GtIntelliGraphConnection*> connections();
    QList<GtIntelliGraphConnection const*> connections() const;

    GtIntelliGraphNode* findNode(QtNodeId nodeId);
    GtIntelliGraphNode const* findNode(QtNodeId nodeId) const;

    GtIntelliGraphConnection* findConnection(QtConnectionId const& conId);
    GtIntelliGraphConnection const* findConnection(QtConnectionId const& conId) const;

    GtIgGroupInputProvider* inputProvider();
    GtIgGroupInputProvider const* inputProvider() const;

    GtIgGroupOutputProvider* outputProvider();
    GtIgGroupOutputProvider const* outputProvider() const;

    void insertOutData(PortIndex idx);

    bool setOutData(PortIndex idx, NodeData data);

    /**
     * @brief Clears all nodes and connections
     */
    void clear();

    /**
     * @brief Removes all nodes and connections not part of the graph model
     * @param model data flow graph model
     */
    void removeOrphans(DataFlowGraphModel& model);

    /**
     * @brief Attemps to restore the intelli graph using the json data
     * @param json Json object describing the intelli graph scene
     * @return Success
     */
    bool fromJson(QJsonObject const& json);

    /**
     * @brief Serializes the whole intelli graph tree as a json object
     * @param clone Whether to clone the object data (i.e. use the same uuid).
     * Only set to true if the object should be restored instead of copied.
     * @return Json object
     */
    QJsonObject toJson(bool clone = false) const;

    /**
     * @brief Creates a new node using the node id in the active graph model as
     * a child object and returns a pointer to it. Returns null if the process
     * failed. The ownership is taken care of. Make sure to set
     * the active graph model beforehand.
     * @param nodeId Node id to create/move
     * @return Node (may be null)
     */
    GtIntelliGraphNode* createNode(QtNodeId nodeId);

    /**
     * @brief Appends the node to the intelli graph. Use this function instead
     * of appending the child directly. Node may change its nodeid if its
     * already occupied
     * @param node Node to append
     * @return success
     */
    bool appendNode(std::unique_ptr<GtIntelliGraphNode> node);

    /**
     * @brief Deletes the node described by node id. Returns true on success.
     * The graph model must not be set beforehand.
     * @param nodeId Node to delete
     * @return True if successful else false
     */
    bool deleteNode(QtNodeId nodeId);

    /**
     * @brief Creates a new connection base on the connection details and
     * returns a pointer to the newly created object (null if the process
     * failed). The ownership is taken care of. The graph model must not be set
     * beforehand.
     * @param connectionId Connection details to be used for creating the node.
     * @return Connection (may be null)
     */
    GtIntelliGraphConnection* createConnection(QtConnectionId const& connectionId);

    /**
     * @brief Appends the connection to intelli graph. Use this function instead
     * of appending the child directly. Aborts if the connection already exists
     * @param connection Connection to append
     * @return success
     */
    bool appendConnection(std::unique_ptr<GtIntelliGraphConnection> connection);

    /**
     * @brief Deletes the connection described by the connection details.
     * Returns true on success. The graph model must not be set beforehand.
     * @param nodeId Node to delete
     * @return True if successful else false
     */
    bool deleteConnection(QtConnectionId const& connectionId);

    /**
     * @brief Updates the node position of the node matgched by the node id.
     * Returns true on success. Make sure to set the active graph model
     * beforehand.
     * @param nodeId Node to update
     * @return True if successful else false
     */
    bool updateNodePosition(QtNodeId nodeId);

    /**
     * @brief Sets the active graph model. This is required for synchronizing
     * changes
     * @param model Model
     */
    void setActiveGraphModel(DataFlowGraphModel& model);

    DataFlowGraphModel* activeGraphModel();
    DataFlowGraphModel const* activeGraphModel() const;

    /**
     * @brief Clears the active graph model. Must not be called explicitly as
     * the model will be cleared automatically once its deleted.
     */
    void clearActiveGraphModel();

protected:

    // keeps active graph model up date if a node or connection was restored
    void onObjectDataMerged() override;

    NodeData eval(PortId outId) override;

private:

    /// pointer to active graph model (i.e. mdi item)
    QPointer<DataFlowGraphModel> m_activeGraphModel;

    std::vector<NodeData> m_outData;

    void setupNode(GtIntelliGraphNode& node);

    void setupConnection(GtIntelliGraphConnection& connection);

    bool appendNodeToModel(GtIntelliGraphNode& node);

    bool appendConnectionToModel(GtIntelliGraphConnection& con);

    void initInputOutputProvider();

    void setNewNodeId(GtIntelliGraphNode& node);
};

#endif // GT_INTELLIGRAPH_H
