/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 17.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GRAPHMODEL_H
#define GRAPHMODEL_H

#include <intelli/node.h>
#include <intelli/nodedata.h>

#include <QtNodes/AbstractGraphModel>

#include <QPointer>

namespace intelli
{

class Graph;

class GT_INTELLI_EXPORT GraphExecutionModel : public QObject
{
    Q_OBJECT

public:

    using NodeDataPtr = std::shared_ptr<const NodeData>;

    enum class NodeEvalState
    {
        Evaluated = 0,
        Evaluating,
        ManualEvaluation
    };

    enum class PortDataState
    {
        Outdated = 0,
        Valid,
    };

    struct PortDataEntry
    {
        PortIndex portIdx;
        PortDataState state = PortDataState::Outdated;
        NodeDataPtr data = {};
    };

    struct Entry
    {
        NodeEvalState state = NodeEvalState::Evaluated;
        QVarLengthArray<PortDataEntry, 8> portsIn = {}, portsOut = {};
    };

    GraphExecutionModel(Graph& graph);

    Graph& graph();
    Graph const& graph() const;

    void beginReset();
    void endReset();
    void reset();

    bool autoEvaluate();

    bool invalidateNodeData(NodeId nodeId);
    bool invalidateInNodeData(NodeId nodeId, PortIndex idx);

    NodeDataPtr const& nodeData(NodeId nodeId, PortType type, PortIndex idx) const;

    bool setNodeData(NodeId nodeId, PortType type, PortIndex idx, NodeDataPtr dataPtr);

private:

    QHash<NodeId, Entry> m_data;

    PortDataEntry* findPortDataEntry(NodeId nodeId, PortType type, PortIndex port);
    PortDataEntry const* findPortDataEntry(NodeId nodeId, PortType type, PortIndex port) const;

    QVector<ConnectionId> invalidateNodeData(NodeId nodeId, PortDataEntry& port);

    void triggerNodeExecution(NodeId nodeId);
};

} // namespace intelli

#endif // GRAPHMODEL_H
