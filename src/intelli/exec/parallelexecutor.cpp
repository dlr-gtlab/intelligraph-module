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
                this]() -> std::vector<NodeDataPtr>
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

        auto const& outPorts = clone->ports(PortType::Out);
        auto const& inPorts  = clone->ports(PortType::In);

        // restore states
        auto& p = accessImpl(*clone);
        p.inData = std::move(inData);
        p.outData = std::move(outData);

        // evalutae single port
        if (targetPort != invalid<PortIndex>())
        {
            this->doEvaluate(*clone, targetPort);
            return p.outData;
        }

        // trigger eval if no outport exists
        if (outPorts.empty() && !inPorts.empty())
        {
            this->doEvaluateAndDiscard(*clone);
            return p.outData;
        }

        // iterate over all output ports
        for (PortIndex idx{0}; idx < outPorts.size(); ++idx)
        {
            this->doEvaluate(*clone, idx);
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
