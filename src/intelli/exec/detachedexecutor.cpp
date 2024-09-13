/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/exec/detachedexecutor.h"

#include "intelli/node.h"
#include "intelli/nodedatainterface.h"
#include "intelli/private/utils.h"

#include "gt_utilities.h"
#include "gt_qtutilities.h"
#include "gt_objectfactory.h"
#include "gt_objectmemento.h"

#include <QThreadPool>
#include <QtConcurrent>

using namespace intelli;

//////////////////////////////////////////////////////

/**
 * @brief The DummyDataModel class.
 * Helper class to set and access data of a single node
 */
class DummyDataModel : public NodeDataInterface
{
public:

    explicit DummyDataModel(Node& node) :
        m_node(&node)
    {
        auto const& inPorts  = node.ports(PortType::In);
        auto const& outPorts = node.ports(PortType::Out);

        m_data.portsIn.reserve(inPorts.size());
        m_data.portsOut.reserve(outPorts.size());

        for (auto& port : inPorts)
        {
            m_data.portsIn.push_back({port.id()});
        }

        for (auto& port : outPorts)
        {
            m_data.portsOut.push_back({port.id()});
        }

        exec::setNodeDataInterface(node, *this);
    }

    NodeDataPtrList nodeData(PortType type) const
    {
        assert(m_node);
        auto const& ports = type == PortType::In ? &m_data.portsIn : &m_data.portsOut;

        NodeDataPtrList data;
        std::transform(ports->begin(), ports->end(),
                       std::back_inserter(data),
                       [this, type](auto& port){
            using T = typename NodeDataPtrList::value_type;
            return T{port.id, port.data};
        });
        return data;
    }

    NodeDataSet nodeData(NodeUuid const& nodeUuid, PortId portId) const override
    {
        assert(m_node);
        if (nodeUuid != m_node->uuid())
        {
            gtError() << QObject::tr("DummyDataModel: Failed to access node %1, "
                                     "was expecting node %4!")
                             .arg(nodeUuid, m_node->uuid());
            return {};
        }

        for (auto const* ports : {&m_data.portsIn, &m_data.portsOut})
        {
            auto iter = std::find_if(ports->begin(), ports->end(),
                                     [portId](auto const& port){
                return port.id == portId;
            });
            if (iter != ports->end()) return iter->data;
        }

        gtWarning() << QObject::tr("DummyDataModel: Failed to access data of '%1' (%2), "
                                   "port %4 not found!")
                           .arg(relativeNodePath(*m_node))
                           .arg(m_node->id())
                           .arg(portId);
        return {};
    }

    NodeDataPtrList nodeData(NodeUuid const& nodeUuid, PortType type) const override
    {
        assert(m_node);
        if (nodeUuid != m_node->uuid())
        {
            gtError() << QObject::tr("DummyDataModel: Failed to access node %1, "
                                     "was expecting node %4!")
                             .arg(nodeUuid, m_node->uuid());
            return {};
        }

        return nodeData(type);
    }

    bool setNodeData(PortId portId, NodeDataSet data)
    {
        assert(m_node);
        return setNodeData(m_node->uuid(), portId, std::move(data));
    }

    bool setNodeData(PortType type, NodeDataPtrList const& data)
    {
        assert(m_node);
        for (auto& d : data)
        {
            if (!setNodeData(d.first, std::move(d.second)))
            {
                return false;
            }
        }
        return true;
    }

    bool setNodeData(NodeUuid const& nodeUuid, PortId portId, NodeDataSet data) override
    {
        assert(m_node);
        if (nodeUuid != m_node->uuid())
        {
            gtError() << QObject::tr("DummyDataModel: Failed to access node %1, "
                                     "was expecting node %4!")
                             .arg(nodeUuid, m_node->uuid());
            return false;
        }

        for (auto* ports : {&m_data.portsIn, &m_data.portsOut})
        {
            auto iter = std::find_if(ports->begin(), ports->end(),
                                     [portId](auto const& port){
                return port.id == portId;
            });
            if (iter != ports->end())
            {
                iter->data = std::move(data);
                return true;
            }
        }

        gtWarning() << QObject::tr("DummyDataModel: Failed to set data of %1 (%2:%3), "
                                   "port %4 not found!")
                           .arg(nodeUuid)
                           .arg(m_node->id(), 2)
                           .arg(m_node->caption())
                           .arg(portId);
        return {};
    }

    bool setNodeData(NodeUuid const& nodeUuid, PortType type, NodeDataPtrList const& data) override
    {
        assert(m_node);
        if (nodeUuid != m_node->uuid())
        {
            gtError() << QObject::tr("DummyDataModel: Failed to access node %1, "
                                     "was expecting node %4!")
                             .arg(nodeUuid, m_node->uuid());
            return false;
        }

        return setNodeData(type, data);
    }

private:

    Node* m_node = nullptr;
    data_model::DataItem m_data;
};

//////////////////////////////////////////////////////

/// we want to skip signals that are speicifc to the Node, GtObject and QObject
/// class, therefore we will calculate the offset to the "custom" signals once
int signal_offset(){
    static int const o = [](){
        auto* sourceMetaObject = &Node::staticMetaObject;

        int offset = 0;

        // Iterate through the slots and signals of the sourceObject's meta object
        for (int i = 0; i < sourceMetaObject->methodCount(); ++i)
        {
            QMetaMethod const& sourceMethod = sourceMetaObject->method(i);

            // Check if the method is a signal
            if (sourceMethod.methodType() != QMetaMethod::Signal) continue;

            offset = i;
        }

        offset += 1;

#ifdef GT_INTELLI_DEBUG_NODE_EXEC
        gtTrace().verbose()
            << "[DetachedExecutor]"
            << QObject::tr("Signal offset for derived nodes of '%1' is %2")
                   .arg(sourceMetaObject->className()).arg(offset);
#endif

        return offset;
    }();
    return o;
}

using SignalSignature = QByteArray;

/// searches for all "custom" signals that should be forwarded to the main node
inline QVector<SignalSignature>
findSignalsToConnect(QObject& object)
{
    const QMetaObject* sourceMetaObject = object.metaObject();

    struct SignalData { QByteArray name, params; };

    QVector<SignalData> sourceSignals;

    // Iterate through the slots and signals of the sourceObject's meta object
    for (int i = signal_offset(); i < sourceMetaObject->methodCount(); ++i)
    {
        QMetaMethod const& sourceMethod = sourceMetaObject->method(i);

        // Check if the method is a signal
        if (sourceMethod.methodType() != QMetaMethod::Signal) continue;

#ifdef GT_INTELLI_DEBUG_NODE_EXEC
        gtTrace().verbose()
            << "[DetachedExecutor]"
            << QObject::tr("Connecting custom sginal '%1' of node '%2'")
                   .arg(sourceMethod.name(), sourceMetaObject->className());
#endif

        sourceSignals.push_back({
            sourceMethod.name(),
            sourceMethod.parameterTypes().join(',')
        });
    }

    QVector<SignalSignature> signalsToConnect;
    signalsToConnect.reserve(sourceSignals.size());

    // signals may contain duplicates (i.e. in case of default arguments)
    int _2ndlast = sourceSignals.size() - 1;
    int i = 0;
    for (; i < _2ndlast; ++i)
    {
        auto& first = sourceSignals.at(i);
        auto& next  = sourceSignals.at(i + 1);

        if (first.name == next.name && next.params.isEmpty()) i++;

        signalsToConnect << first.name + '(' + first.params + ')';
    }

    // we may have to connect the last element as well
    if (i == _2ndlast)
    {
        auto& signal = sourceSignals.at(i);
        signalsToConnect << signal.name + '(' + signal.params + ')';
    }

    return signalsToConnect;
}

/// interconnect signals of the copied node to the actual node in the main
/// thread. Only connect "custom" signals
inline bool
connectSignals(QVector<SignalSignature> const& signalsToConnect,
               QPointer<Node> sourceObject,
               QMetaObject const* sourceMetaObject,
               QPointer<Node> targetObject,
               QMetaObject const* targetMetaObject,
               QPointer<QObject> executor)
{
    // connect signals cloned object with original object
    for (SignalSignature const& signal : qAsConst(signalsToConnect))
    {
        int signalIndex = sourceMetaObject->indexOfSignal(signal);
        if (signalIndex == -1)
        {
            gtWarning()
                << GT_CLASSNAME(DetachedExecutor) << '-'
                << QObject::tr("Failed to forward signal from clone to source node!")
                << gt::brackets(signal);
            return {};
        }
        assert(signalIndex == targetMetaObject->indexOfSignal(signal));

#ifdef GT_INTELLI_DEBUG_NODE_EXEC
        gtTrace().verbose()
            << QStringLiteral("[DetachedExecutor]")
            << QObject::tr("Connecting custom Node signal '%1'").arg(signal.constData());
#endif

        if (!QObject::connect(sourceObject, sourceMetaObject->method(signalIndex),
                              targetObject, targetMetaObject->method(signalIndex),
                              Qt::QueuedConnection))
        {
            gtWarning()
                << GT_CLASSNAME(DetachedExecutor) << '-'
                << QObject::tr("Failed to connect signal of clone with source node!")
                << gt::brackets(signal);
            return {};
        }
    }

    // destroy connections if the executor is destroyed
    auto success = QObject::connect(executor, &QObject::destroyed,
                                    sourceObject, [targetObject, sourceObject](){
        if (sourceObject && targetObject) sourceObject->disconnect(targetObject);
    });
    return success;
}

//////////////////////////////////////////////////////

DetachedExecutor::DetachedExecutor()
{
    using Watcher= decltype(m_watcher);

    connect(&m_watcher, &Watcher::finished,
            this, &DetachedExecutor::onFinished);
    connect(&m_watcher, &Watcher::canceled,
            this, &DetachedExecutor::onCanceled);
    connect(&m_watcher, &Watcher::resultReadyAt,
            this, &DetachedExecutor::onResultReady);
}

bool
DetachedExecutor::canEvaluateNode(Node& node)
{
    if (!m_watcher.isFinished() || !m_collected)
    {
        gtWarning() << tr("Cannot evaluate node '%1'! (Node is already running)")
                           .arg(node.objectName());
        return false;
    }
    return true;
}

DetachedExecutor::~DetachedExecutor() = default;

void
DetachedExecutor::onFinished()
{
    if (!m_node)
    {
        gtError() << QObject::tr("Cannot finish transfer of node data! "
                                 "(Invalid node)");
        return;
    }

    m_collected = true;

    connect(this, &QObject::destroyed, m_node, &Node::computingFinished);

    // commit suicide
    deleteLater();
}

void
DetachedExecutor::onCanceled()
{
    gtError() << tr("Execution of node '%1' failed!")
                     .arg(m_node ? m_node->objectName() : QStringLiteral("<null>"));
}

void
DetachedExecutor::onResultReady(int result)
{
    if (!m_node)
    {
        gtError() << tr("Cannot transfer node data! (Invalid node)");
        return;
    }

    auto finally = [this](){
        m_collected = true;
        emit m_node->computingFinished();
    };
    Q_UNUSED(finally);

#ifdef GT_INTELLI_DEBUG_NODE_EXEC
    gtTrace().verbose()
        << QStringLiteral("[DetachedExecutor]")
        << QObject::tr("collecting data from node '%1' (%2)...")
               .arg(relativeNodePath(*m_node))
               .arg(m_node->id());
#endif

    auto const& outData = m_watcher.resultAt(result);

    auto* model = exec::nodeDataInterface(*m_node);
    if (!model)
    {
        gtError() << tr("Failed to transfer node data!")
                  << tr("(Execution model not found)");
        return;
    }

    if (!model->setNodeData(m_node->uuid(), PortType::Out, outData))
    {
        gtError() << tr("Failed to transfer node data!");
        return;
    }
}

bool
DetachedExecutor::evaluateNode(Node& node, NodeDataInterface& model)
{
    if (!canEvaluateNode(node)) return false;

    m_node = &node;
    m_collected = false;
    emit m_node->computingStarted();

    NodeUuid const& nodeUuid = node.uuid();

    auto inData  = model.nodeData(nodeUuid, PortType::In);
    auto outData = model.nodeData(nodeUuid, PortType::Out);

    auto run = [nodeUuid,
                inData  = model.nodeData(nodeUuid, PortType::In),
                outData = model.nodeData(nodeUuid, PortType::Out),
                memento = node.toMemento(),
                signalsToConnect = findSignalsToConnect(node),
                targetMetaObject = node.metaObject(),
                targetObject = QPointer<Node>(&node),
                executor = this]() -> NodeDataPtrList
    {
#ifdef GT_INTELLI_DEBUG_NODE_EXEC
        gtTrace().verbose()
            << QStringLiteral("[DetachedExecutor]")
            << QObject::tr("beginning evaluation of node '%1' (%2)...")
                   .arg(memento.ident())
                   .arg(nodeUuid);
#endif

        auto const makeError = [nodeUuid](){
            return tr("Evaluating node %1 failed!").arg(nodeUuid);
        };

        try{
            auto clone = gt::unique_qobject_cast<Node>(
                memento.toObject(*gtObjectFactory)
            );

            auto* node = clone.get();
            if (!node)
            {
                gtError() << makeError()
                          << tr("(Cloning node failed)").arg(memento.ident());
                return {};
            }

            assert(node->ports(PortType::Out).size() == outData.size());
            assert(node->ports(PortType::In).size()  == inData.size());

            if (!signalsToConnect.empty())
            {
                if (!connectSignals(signalsToConnect,
                                    node, node->metaObject(),
                                    targetObject, targetMetaObject,
                                    executor))
                {
                    return {};
                }
            }

            // set data
            DummyDataModel model{*node};

            bool success = true;
            success &= model.setNodeData(PortType::In,  inData);
            success &= model.setNodeData(PortType::Out, outData);

            if (!success)
            {
                gtError() << makeError() << tr("(failed to copy source data)");
                return {};
            }

            // evaluate node
            exec::blockingEvaluation(*node, model);

            return model.nodeData(PortType::Out);
        }
        catch (const std::exception& ex)
        {
            gtError() << makeError()
                      << tr("(caught exception: %1)").arg(ex.what());
            return {};
        }
        catch(...)
        {
            gtError() << makeError() << tr("(caught unkown exception)");
            return {};
        }
    };

    auto* pool = QThreadPool::globalInstance();
    auto future = QtConcurrent::run(pool, std::move(run));
    m_watcher.setFuture(future);

    return true;
}
