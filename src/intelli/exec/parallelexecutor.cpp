/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/exec/parallelexecutor.h"

#include "intelli/node.h"
#include "intelli/private/node_impl.h"

#include "gt_qtutilities.h"
#include "gt_objectfactory.h"
#include "gt_objectmemento.h"

#include <QThreadPool>
#include <QtConcurrent>

using namespace intelli;

ParallelExecutor::ParallelExecutor()
{
    using Watcher= decltype(m_watcher);

    connect(&m_watcher, &Watcher::finished,
            this, &ParallelExecutor::onFinished);
    connect(&m_watcher, &Watcher::canceled,
            this, &ParallelExecutor::onCanceled);
    connect(&m_watcher, &Watcher::resultReadyAt,
            this, &ParallelExecutor::onResultReady);
}

bool
ParallelExecutor::canEvaluateNode(Node& node, PortIndex outIdx)
{
    if (!m_watcher.isFinished() || !m_collected)
    {
        gtWarning() << tr("Cannot evaluate node '%1'! (Node is already running)")
                           .arg(node.objectName());
        return false;
    }
    return Executor::canEvaluateNode(node, outIdx);

}

ParallelExecutor::~ParallelExecutor()
{
    if (!ParallelExecutor::isReady())
    {
        gtWarning().verbose() << __func__ << "is not ready for deletion!";
    }
}

void
ParallelExecutor::onFinished()
{
    if (!m_node)
    {
        gtError() << QObject::tr("Cannot finish transfer of node data! "
                                 "(Invalid node)");
        return;
    }

//    m_node.clear();
    m_collected = true;
    emit m_node->computingFinished();

    auto& pimpl = accessImpl(*m_node);
    if (pimpl.requiresEvaluation)
    {
        m_node->updateNode();
    }
}

void
ParallelExecutor::onCanceled()
{
    gtWarning().verbose() << __func__ << m_node;
}

void
ParallelExecutor::onResultReady(int idx)
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

    std::vector<NodeDataPtr> outData = m_watcher.resultAt(idx);

    auto& p = accessImpl(*m_node);

    if (p.outData.size() != outData.size())
    {
        gtError() << tr("Cannot transfer node data! "
                        "(Data size does not match: expected %1, got %2)")
                         .arg(outData.size()).arg(p.outData.size());
        return;
    }

    p.outData = std::move(outData);

    if (p.outData.empty())
    {
        return emit m_node->evaluated();
    }

    auto const emitOutDataUpdated = [&p, this](PortIndex idx){
        auto& out = p.outData.at(idx);

        emit m_node->evaluated(idx);

        out ? emit m_node->outDataUpdated(idx) :
              emit m_node->outDataInvalidated(idx);
    };

    if (m_port != invalid<PortIndex>())
    {
        return emitOutDataUpdated(m_port);
    }
    for (PortIndex outIdx{0}; outIdx < p.outPorts.size(); ++outIdx)
    {
        emitOutDataUpdated(outIdx);
    }
}

bool
ParallelExecutor::evaluateNode(Node& node)
{
    m_port = invalid<PortIndex>();
    return evaluateNodeHelper(node);
}

bool
ParallelExecutor::evaluatePort(Node& node, PortIndex idx)
{
    m_port = idx;
    return evaluateNodeHelper(node);
}

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
    return offset;
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
               QPointer<Executor> executor)
{
    // connect signals cloned object with original object
    for (SignalSignature const& signal : qAsConst(signalsToConnect))
    {
        int signalIndex = sourceMetaObject->indexOfSignal(signal);
        if (signalIndex == -1)
        {
            gtWarning()
                << QObject::tr("Failed to forward signal from clone to source node!")
                << gt::brackets(signal);
            return {};
        }
        assert(signalIndex == targetMetaObject->indexOfSignal(signal));

        gtInfo().verbose()
            << QObject::tr("Connecting custom Node signal '%1'").arg(signal.constData());

        if (!QObject::connect(sourceObject, sourceMetaObject->method(signalIndex),
                              targetObject, targetMetaObject->method(signalIndex),
                              Qt::QueuedConnection))
        {
            gtWarning()
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

bool
ParallelExecutor::evaluateNodeHelper(Node& node)
{
    if (!canEvaluateNode(node)) return false;

    m_node = &node;
    m_collected = false;
    emit m_node->computingStarted();

    auto& p = accessImpl(node);

    auto run = [targetPort = m_port,
                inData = p.inData,
                outData = p.outData,
                memento = node.toMemento(),
                signalsToConnect = findSignalsToConnect(node),
                targetMetaObject = node.metaObject(),
                targetObject = QPointer<Node>(&node),
                executor = this]() -> std::vector<NodeDataPtr>
    {
        auto clone = gt::unique_qobject_cast<Node>(
            memento.toObject(*gtObjectFactory)
        );

        if (!clone)
        {
            gtError() << tr("Failed to clone node '%1'")
                             .arg(memento.ident());
            return {};
        }

        if (!signalsToConnect.empty())
        {
            if (!connectSignals(signalsToConnect,
                                clone.get(), clone->metaObject(),
                                targetObject, targetMetaObject,
                                executor))
            {
                return {};
            }
        }

        auto const& outPorts = clone->ports(PortType::Out);
        auto const& inPorts  = clone->ports(PortType::In);

        // restore states
        auto& p = accessImpl(*clone);
        p.inData = std::move(inData);
        p.outData = std::move(outData);

        // evalutae single port
        if (targetPort != invalid<PortIndex>())
        {
            doEvaluate(*clone, targetPort);
            return p.outData;
        }

        // trigger eval if no outport exists
        if (outPorts.empty() && !inPorts.empty())
        {
            doEvaluateAndDiscard(*clone);
            return p.outData;
        }

        // iterate over all output ports
        for (PortIndex idx{0}; idx < outPorts.size(); ++idx)
        {
            doEvaluate(*clone, idx);
        }

        return p.outData;
    };

    auto* pool = QThreadPool::globalInstance();
    auto future = QtConcurrent::run(pool, std::move(run));
    m_watcher.setFuture(future);

    return true;
}

bool
ParallelExecutor::isReady() const
{
    return m_watcher.isCanceled() || m_watcher.isFinished();
}
