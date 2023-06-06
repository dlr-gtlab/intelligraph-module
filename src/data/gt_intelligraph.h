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

#include "gt_object.h"

#include <QPointer>

namespace QtNodes { class DataFlowGraphModel; }

class QJsonObject;

class GtIntelliGraphNode;
class GtIntelliGraphConnection;
class GtIntelliGraphObjectModel;
class GtIntelliGraph : public GtObject
{
    Q_OBJECT

public:

    using DataFlowGraphModel = QtNodes::DataFlowGraphModel;

    using NodeId = QtNodes::NodeId;
    using ConnectionId = QtNodes::ConnectionId;

    Q_INVOKABLE GtIntelliGraph();

    QList<GtIntelliGraphNode*> nodes();
    QList<GtIntelliGraphNode const*> nodes() const;

    QList<GtIntelliGraphConnection*> connections();
    QList<GtIntelliGraphConnection const*> connections() const;

    GtIntelliGraphNode* findNode(NodeId nodeId);
    GtIntelliGraphNode const* findNode(NodeId nodeId) const;

    GtIntelliGraphConnection* findConnection(ConnectionId const& conId);
    GtIntelliGraphConnection const* findConnection(ConnectionId const& conId) const;

    /// clears all nodes and connections
    void clear();

    /// removes all nodes and connections not part of the graph model
    void removeOrphans(DataFlowGraphModel& model);

    /// attemps to restore the intelli graph using the json data
    bool fromJson(QJsonObject const& json);

    QJsonObject toJson() const;

    /**
     * @brief Creates a new node from the node in the active graph model as a
     * child object and returns a pointer to it. Returns null if the process
     * failed. The ownership is taken care of. Make sure to set
     * the active graph model beforehand.
     * @param nodeId Node id to create/move
     * @return Node (may be null)
     */
    GtIntelliGraphNode* createNode(NodeId nodeId);

    /**
     * @brief Appends the node  to the intelli graph. Use this function instead
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
    bool deleteNode(NodeId nodeId);

    /**
     * @brief Creates a new connection base on the connection details and
     * returns a poiinter to it. Returns null if the process failed. The
     * ownership is taken care of. The graph model must not be set beforehand.
     * @param connectionId Connection details to be used for creating the node.
     * @return Connection (may be null)
     */
    GtIntelliGraphConnection* createConnection(ConnectionId const& connectionId);

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
    bool deleteConnection(ConnectionId const& connectionId);

    /**
     * @brief Updates the node position of the node matgched by the node id.
     * Returns true on success. Make sure to set the active graph model
     * beforehand.
     * @param nodeId Node to update
     * @return True if successful else false
     */
    bool updateNodePosition(NodeId nodeId);

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

    // keeps active graph model up date if a node or connection was changed
    void onObjectDataMerged() override;

private:

    /// pointer to active graph model (i.e. mdi item)
    QPointer<DataFlowGraphModel> m_activeGraphModel;

    void setupNode(GtIntelliGraphNode& node);

    void setupConnection(GtIntelliGraphConnection& connection);

    bool appendNodeToModel(GtIntelliGraphNode& node);

    bool appendConnectionToModel(GtIntelliGraphConnection& con);
};

#endif // GT_INTELLIGRAPH_H
