/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 18.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTINTELLIGRAPHMODELMANAGER_H
#define GTINTELLIGRAPHMODELMANAGER_H

#include "gt_igglobals.h"
#include "gt_igvolatileptr.h"

#include <QObject>

#include <QtNodes/DataFlowGraphModel>

class GtIntelliGraph;
class GtIntelliGraphNode;
class GtIntelliGraphConnection;
class GtIntelliGraphModelManager : public QObject
{
    Q_OBJECT

public:

    GtIntelliGraphModelManager(GtIntelliGraph& parent,
                               gt::ig::ModelPolicy policy);

    GtIntelliGraph* intelliGraph();
    GtIntelliGraph const* intelliGraph() const;

    gt::ig::ModelPolicy policy() const;

    void updatePolicy(gt::ig::ModelPolicy policy);

    bool readyForRemoval(bool force = true) const;

    void mergeGraph(GtIntelliGraph& ig);

    /**
     * @brief Returns the active graph model
     * @return Graph model
     */
    QtNodes::DataFlowGraphModel* graphModel();
    QtNodes::DataFlowGraphModel const* graphModel() const;

    /**
     * @brief Creates a new node using the node id in the active graph model as
     * a child object and returns a pointer to it. Returns null if the process
     * failed. The ownership is taken care of. Make sure to set
     * the graph model beforehand.
     * @param nodeId Node id to create/move
     * @return Node (may be null)
     */
    bool appendNodeFromModel(QtNodes::NodeId nodeId);

    /**
     * @brief Creates a new connection base on the connection details and
     * returns a pointer to the newly created object (null if the process
     * failed). The ownership is taken care of. The graph model must not be set
     * beforehand.
     * @param connectionId Connection details to be used for creating the node.
     * @return Connection (may be null)
     */
    bool appendConnectionFromModel(QtNodes::ConnectionId connectionId);

public slots:

    bool appendNodeToModel(GtIntelliGraphNode* node);

    bool appendConnectionToModel(GtIntelliGraphConnection* connection);

private:

    mutable gt::ig::ModelPolicy m_policy = gt::ig::DummyModel;

    /// pointer to active graph model (i.e. mdi item)
    gt::ig::volatile_ptr<QtNodes::DataFlowGraphModel> m_graphModel;

    /**
     * @brief Removes all nodes and connections not part of the graph model.
     * The graph model must be set beforehand.
     */
    void removeOrphans(GtIntelliGraph& ig);

    void setupNode(GtIntelliGraphNode& node);

    void setupConnection(GtIntelliGraphConnection& connection);
};



#endif // GTINTELLIGRAPHMODELMANAGER_H
