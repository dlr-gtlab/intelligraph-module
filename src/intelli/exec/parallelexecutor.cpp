/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/exec/parallelexecutor.h"

#include "intelli/graph.h"
#include "intelli/node.h"
#include "intelli/graphexecmodel.h"
#include "intelli/private/node_impl.h"

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
ParallelExecutor::canEvaluateNode(Node& node)
{
    if (!m_watcher.isFinished() || !m_collected)
    {
        gtWarning() << tr("Cannot evaluate node '%1'! (Node is already running)")
                           .arg(node.objectName());
        return false;
    }
    return true;
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

    deleteLater();
}

void
ParallelExecutor::onCanceled()
{
    gtWarning().verbose() << __func__ << m_node;
}

void
ParallelExecutor::onResultReady(int result)
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

    auto* model = accessExecModel(*m_node);
    if (!model)
    {
        gtError() << tr("Failed to transfer node data!")
                  << tr("(Execution model not found)");
        return;
    }

    bool success = model->setNodeData(m_node->id(), PortType::Out, outData);
    if (!success)
    {
        gtError() << tr("Failed to transfer node data!");
        return;
    }

    if (outData.empty())
    {
        return emit m_node->evaluated();
    }

    auto const emitOutDataUpdated = [this](PortId port){
        emit m_node->evaluated(port);
    };

    if (m_port != invalid<PortId>())
    {
        return emitOutDataUpdated(m_port);
    }
    for (auto& data : outData)
    {
        emitOutDataUpdated(m_node->portId(PortType::Out, data.first));
    }
}

bool
ParallelExecutor::evaluateNode(Node& node, GraphExecutionModel& model, PortId portId)
{
    m_port = portId;

    if (!canEvaluateNode(node)) return false;

    m_node = &node;
    m_collected = false;
    emit m_node->computingStarted();

    auto run = [targetPort = m_port,
                inData  = model.nodeData(node.id(), PortType::In),
                outData = model.nodeData(node.id(), PortType::Out),
                memento = node.toMemento(),
                signalsToConnect = findSignalsToConnect(node),
                targetMetaObject = node.metaObject(),
                targetObject = QPointer<Node>(&node),
                executor = this]() -> NodeDataPtrList
    {
        try{
        auto clone = gt::unique_qobject_cast<Node>(
            memento.toObject(*gtObjectFactory)
        );

        auto* node = clone.get();
        if (!node)
        {
            gtError() << tr("Failed to clone node '%1'")
                             .arg(memento.ident());
            return {};
        }

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

        Graph graph;
        if (!graph.appendNode(std::move(clone))) return {};

        GraphExecutionModel model(graph);

        auto const& outPorts = node->ports(PortType::Out);
        auto const& inPorts  = node->ports(PortType::In);

        assert(outPorts.size() == outData.size());
        assert(inPorts.size()  == inData.size());

        // restore states
        bool success = true;
        success &= model.setNodeData(node->id(), PortType::In,  inData);
        success &= model.setNodeData(node->id(), PortType::Out, outData);

        if (!success) return {};

        // evaluate single port
        if (targetPort != invalid<PortId>())
        {
            model.setNodeData(node->id(), targetPort, doEvaluate(*node, targetPort));
            return model.nodeData(node->id(), PortType::Out);
        }

        // trigger eval if no outport exists
        if (outPorts.empty() && !inPorts.empty())
        {
            doEvaluate(*node);
            return {};
        }

        // iterate over all output ports
        for (auto& port : outPorts)
        {
            model.setNodeData(node->id(), port.id(), doEvaluate(*node, port.id()));
        }

        return model.nodeData(node->id(), PortType::Out);
        }
        catch(...)
        {
        gtError() << "HERE: something went wrong";
        std::exception_ptr ex_ptr = std::current_exception();

        // Now you can rethrow or inspect the exception
        try {
            if (ex_ptr) {
                std::rethrow_exception(ex_ptr); // Rethrow the exception
            }
        }
        catch (const std::bad_alloc& ex) {
            gtError() << "Caught std::bad_alloc: " << ex.what() << std::endl;
        }
        catch (const std::runtime_error& ex) {
            gtError() << "Caught std::runtime_error: " << ex.what() << std::endl;
        }
        catch (const std::exception& ex) {
            gtError() << "Caught std::exception: " << ex.what() << std::endl;
        }
        catch (...) {
            gtError() << "Caught an unknown exception" << std::endl;
        }
        assert(false);
        return {};
        }
    };

    auto* pool = QThreadPool::globalInstance();
    auto future = QtConcurrent::run(pool, std::move(run));
    m_watcher.setFuture(future);

    future.waitForFinished();

    return true;
}

bool
ParallelExecutor::isReady() const
{
    return m_watcher.isCanceled() || m_watcher.isFinished();
}
