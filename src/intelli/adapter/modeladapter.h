/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 18.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_MODELADAPTER_H
#define GT_INTELLI_MODELADAPTER_H

#include "intelli/globals.h"
#include "intelli/memory.h"

#include <QObject>

#include <QtNodes/DataFlowGraphModel>

namespace intelli
{

class Graph;
class Node;
class Connection;

class ModelAdapter : public QObject
{
    Q_OBJECT

public:
    
    ModelAdapter(Graph& parent, ModelPolicy policy);
    ~ModelAdapter();
    
    Graph& intelliGraph();
    Graph const& intelliGraph() const;

    /**
     * @brief Returns the active graph model
     * @return Graph model
     */
    QtNodes::DataFlowGraphModel* graphModel();
    QtNodes::DataFlowGraphModel const* graphModel() const;

    /**
     * @brief Getter for the model policy.
     * @return
     */
    ModelPolicy modelPolicy() const { return m_policy; }

    /**
     * @brief Setter for the model policy
     * @param policy New policy
     */
    void setModelPolicy(ModelPolicy policy) { m_policy = policy; }

    bool readyForRemoval(bool force = true) const;
    
    void mergeConnections(Graph& ig);
    
    void mergeGraphModel(Graph& ig);

public slots:

    /**
     * @brief Moves the node specified by nodeId of the graph model to the
     * intelli graph data object.
     * @param nodeId Node id to move
     * @return Succes. Will fail if node was already moved
     */
    bool appendNodeFromModel(QtNodes::NodeId nodeId);

    /**
     * @brief Creates a new connection based on the connection id and appends it
     * to the intelli graph data object
     * @param connectionId Connection to append.
     * @return Succes. Will fail if connection was already created
     */
    bool appendConnectionFromModel(QtNodes::ConnectionId conId);

    /**
     * @brief Appends the node to the graph model.
     * @param node Node to append to the graph model
     * @return Success. Will fail if the node id already exists
     */
    bool appendNodeToModel(Node* node);

    /**
     * @brief Appends the connection to the graph model.
     * @param connection Connection to append to the graph model
     * @return Success. Will fail if the connection id already exists
     */
    bool appendConnectionToModel(Connection* connection);

private:

    /// policy of this adapter (i.e. wheter it is a dummy or an active model)
    ModelPolicy m_policy = DummyModel;

    /// pointer to active graph model (i.e. mdi item)
    volatile_ptr<QtNodes::DataFlowGraphModel> m_graphModel;

    /**
     * @brief Removes all nodes and connections not part of the graph model.
     * The graph model must be set beforehand.
     */
    [[deprecated("unused")]]
    void removeOrphans(Graph& ig);

    /**
     * @brief Setups the node and all its signals etc.
     * @param node Node to setup
     */
    void setupNode(Node& node);

    /**
     * @brief Setups the connection and all its signals etc.
     * @param connection Connection to setup
     */
    void setupConnection(Connection& connection);

private slots:

    void onNodeDeletedFromModel(QtNodes::NodeId nodeId);

    void onConnectionDeletedFromModel(QtNodes::ConnectionId conId);
};

} // namespace intelli;

using GtIntelliGraphModelAdapter [[deprecated]] = intelli::ModelAdapter;

#endif // GT_INTELLI_MODELADAPTER_H
