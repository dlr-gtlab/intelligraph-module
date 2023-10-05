/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/exec/detachedexecutor.h"

#include "intelli/node.h"
#include "intelli/graphexecmodel.h"

#include "gt_utilities.h"
#include "gt_qtutilities.h"
#include "gt_objectfactory.h"
#include "gt_objectmemento.h"

#include <QThreadPool>
#include <QtConcurrent>

using namespace intelli;

// we want to skip signals that are speicifc to Node, GtObject and QObject,
// therefore we will calculate the offset to the "custom" signals once
static int const s_signal_offset = [](){
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
    return offset + 1;
}();

using SignalSignature = QByteArray;

inline QVector<SignalSignature>
findSignalsToConnect(QObject& object)
{
    const QMetaObject* sourceMetaObject = object.metaObject();

    struct SignalData { QByteArray name, params; };

    QVector<SignalData> sourceSignals;

    // Iterate through the slots and signals of the sourceObject's meta object
    for (int i = s_signal_offset; i < sourceMetaObject->methodCount(); ++i)
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

        gtDebug().verbose()
            << GT_CLASSNAME(DetachedExecutor) << '-'
            << QObject::tr("Connecting custom Node signal '%1'").arg(signal.constData());

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
    emit m_node->computingFinished();

    // commit suicide
    deleteLater();
}

void
DetachedExecutor::onCanceled()
{
    gtError() << tr("Execution of node '%1' failed!")
                     .arg(m_node ? m_node->objectName() : QStringLiteral("NULL"));
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

    auto outData = m_watcher.resultAt(result);

    auto* model = NodeExecutor::accessExecModel(*m_node);
    if (!model)
    {
        gtError() << tr("Failed to transfer node data!")
                  << tr("(Execution model not found)");
        return;
    }

    if (!model->setNodeData(m_node->id(), PortType::Out, outData))
    {
        gtError() << tr("Failed to transfer node data!");
        return;
    }
}

bool
DetachedExecutor::evaluateNode(Node& node, GraphExecutionModel& model)
{
    if (!canEvaluateNode(node)) return false;

    m_node = &node;
    m_collected = false;
    emit m_node->computingStarted();

    auto run = [nodeId = node.id(),
                inData  = model.nodeData(node.id(), PortType::In),
                outData = model.nodeData(node.id(), PortType::Out),
                memento = node.toMemento(),
                signalsToConnect = findSignalsToConnect(node),
                targetMetaObject = node.metaObject(),
                targetObject = QPointer<Node>(&node),
                executor = this]() -> NodeDataPtrList
    {
        auto const makeError = [nodeId](){
            return tr("Evaluating node %1 failed!").arg(nodeId);
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
            DummyDataModel model(*node);

            bool success = true;
            success &= model.setNodeData(PortType::In,  inData);
            success &= model.setNodeData(PortType::Out, outData);

            if (!success)
            {
                gtError() << makeError() << tr("(failed to copy source data9");
                return {};
            }

            // evaluate node
            NodeExecutor::doEvaluate(*node);

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
