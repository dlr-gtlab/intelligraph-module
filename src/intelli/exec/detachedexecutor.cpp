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
#include "intelli/exec/dummynodedatamodel.h"
#include "intelli/private/utils.h"

#include "gt_utilities.h"
#include "gt_qtutilities.h"
#include "gt_objectfactory.h"
#include "gt_objectmemento.h"

#include <QThreadPool>
#include <QtConcurrent>

using namespace intelli;

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
            << utils::logId<DetachedExecutor>()
            << QObject::tr("signal offset for derived nodes of '%1' is %2")
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
                << utils::logId<DetachedExecutor>()
                << QObject::tr("Failed to forward signal from clone to source node!")
                << gt::brackets(signal);
            return {};
        }
        assert(signalIndex == targetMetaObject->indexOfSignal(signal));

#ifdef GT_INTELLI_DEBUG_NODE_EXEC
        gtTrace().verbose()
            << utils::logId<DetachedExecutor>()
            << QObject::tr("connecting custom signal '%1' of node '%2'")
                   .arg(signal, sourceMetaObject->className());
#endif

        if (!QObject::connect(sourceObject, sourceMetaObject->method(signalIndex),
                              targetObject, targetMetaObject->method(signalIndex),
                              Qt::QueuedConnection))
        {
            gtWarning()
                << utils::logId<DetachedExecutor>()
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

DetachedExecutor::DetachedExecutor(QObject* parent) :
    QObject(parent)
{
    using Watcher = decltype(m_watcher);

    connect(&m_watcher, &Watcher::finished,
            this, &DetachedExecutor::onFinished);
    connect(&m_watcher, &Watcher::canceled,
            this, &DetachedExecutor::onCanceled);
    connect(&m_watcher, &Watcher::resultReadyAt,
            this, &DetachedExecutor::onResultReady);
}

DetachedExecutor::~DetachedExecutor() = default;

bool
DetachedExecutor::canEvaluateNode()
{
    return m_collected && !m_destroyed;
}

void
DetachedExecutor::onFinished()
{
    if (!m_node)
    {
        gtError() << utils::logId(this)
                  << tr("Failed to finalize node data transfer! (Invalid node)");

        m_destroyed = true;
        return deleteLater();
    }

    if (m_watcher.isRunning()) return;

    m_destroyed = true;
    deleteLater();
}

void
DetachedExecutor::onCanceled()
{
    gtError() << utils::logId(this)
              << tr("Execution of node '%1' failed!")
                     .arg(m_node ? m_node->objectName() :
                                   QStringLiteral("<null>"));
}

void
DetachedExecutor::onResultReady(int result)
{
    if (!m_node)
    {
        gtError() << utils::logId(this)
                  << tr("Failed to transfer node data! (Invalid node)");
        return;
    }

    auto finally = gt::finally([this](){
        if (m_node) emit m_node->computingFinished();
    });
    Q_UNUSED(finally);

#ifdef GT_INTELLI_DEBUG_NODE_EXEC
    gtTrace().verbose()
        << utils::logId(this)
        << tr("collecting data from node '%1' (%2)...")
               .arg(relativeNodePath(*m_node))
               .arg(m_node->id());
#endif

    ReturnValue const& returnValue = m_watcher.resultAt(result);
    auto const& outData = returnValue.data;

    m_collected = true;

    auto* model = exec::nodeDataInterface(*m_node);
    if (!model)
    {
        gtError() << utils::logId(this)
                  << tr("Failed to transfer node data! (Missing data interface)");
        return;
    }

    NodeUuid const& nodeUuid = m_node->uuid();

    if (!returnValue.success) model->setNodeEvaluationFailed(nodeUuid);

    if (!model->setNodeData(nodeUuid, PortType::Out, outData))
    {
        gtError() << utils::logId(this)
                  << tr("Failed to transfer node data!");
    }

    finally.finalize();

    model->nodeEvaluationFinished(nodeUuid);
}

bool
DetachedExecutor::evaluateNode(Node& node)
{
    NodeDataInterface* model = exec::nodeDataInterface(node);
    assert(model);

    if (!canEvaluateNode())
    {
        gtWarning() << utils::logId(this)
                    << tr("Failed to evaluate node '%1'! (Node is already running)")
                           .arg(node.objectName());
        return false;
    }

#ifdef GT_INTELLI_DEBUG_NODE_EXEC
    if (!m_watcher.isFinished())
    {
        gtTrace().verbose()
            << utils::logId(this)
            << tr("reusing executor:") << (void*)this;
    }
#endif

    model->nodeEvaluationStarted(node.uuid());

    m_node = &node;
    m_collected = false;
    emit m_node->computingStarted();

    NodeUuid const& nodeUuid = node.uuid();

    auto run = [nodeUuid,
                inData  = model->nodeData(nodeUuid, PortType::In),
                outData = model->nodeData(nodeUuid, PortType::Out),
                memento = node.toMemento(),
                signalsToConnect = findSignalsToConnect(node),
                targetMetaObject = node.metaObject(),
                targetObject = QPointer<Node>(&node),
                executor = this]() -> ReturnValue
    {
#ifdef GT_INTELLI_DEBUG_NODE_EXEC
        gtTrace().verbose()
            << utils::logId<DetachedExecutor>()
            << tr("beginning evaluation of node '%1' (%2)...")
                   .arg(memento.ident())
                   .arg(nodeUuid);
#endif

        auto const makeError = [nodeUuid](){
            return tr("evaluating node %1 failed!").arg(nodeUuid);
        };

        try{
            auto clone = gt::unique_qobject_cast<Node>(
                memento.toObject(*gtObjectFactory)
            );

            auto* node = clone.get();
            if (!node)
            {
                gtError() << utils::logId<DetachedExecutor>() << makeError()
                          << tr("(cloning node failed)").arg(memento.ident());
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
            DummyNodeDataModel model{*node};

            bool success = true;
            success &= model.setNodeData(PortType::In,  inData);
            success &= model.setNodeData(PortType::Out, outData);

            if (!success)
            {
                gtError() << utils::logId<DetachedExecutor>() << makeError()
                          << tr("(failed to copy source data)");
                return {};
            }

            // evaluate node
            exec::blockingEvaluation(*node);

            return ReturnValue{model.nodeData(PortType::Out), model.evaluationSuccessful()};
        }
        catch (const std::exception& ex)
        {
            gtError() << utils::logId<DetachedExecutor>() << makeError()
                      << tr("(caught exception: %1)").arg(ex.what());
            return {};
        }
        catch(...)
        {
            gtError() << utils::logId<DetachedExecutor>() << makeError()
                      << tr("(caught unkown exception)");
            return {};
        }
    };

    auto* pool = QThreadPool::globalInstance();
    auto future = QtConcurrent::run(pool, std::move(run));
    m_watcher.setFuture(future);

    return true;
}
