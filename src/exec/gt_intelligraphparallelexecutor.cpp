/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphparallelexecutor.h"

#include "gt_intelligraph.h"
#include "gt_intelligraphnode.h"
#include "private/intelligraphnode_impl.h"
#include "private/utils.h"

#include "gt_qtutilities.h"
#include "gt_objectfactory.h"
#include "gt_objectmemento.h"

#include <QThreadPool>
#include <QtConcurrent>

using namespace gt::ig;

GtIntelliGraphParallelExecutor::GtIntelliGraphParallelExecutor()
{
    using Watcher= decltype(m_watcher);

    connect(&m_watcher, &Watcher::finished,
            this, &GtIntelliGraphParallelExecutor::onFinished);
    connect(&m_watcher, &Watcher::canceled,
            this, &GtIntelliGraphParallelExecutor::onCanceled);
    connect(&m_watcher, &Watcher::resultReadyAt,
            this, &GtIntelliGraphParallelExecutor::onResultReady);
}

bool
GtIntelliGraphParallelExecutor::canEvaluateNode(GtIntelliGraphNode& node, PortIndex outIdx)
{
    if (!m_watcher.isFinished() || !m_collected)
    {
        gtWarning() << tr("Cannot evaluate node '%1'! (Node is already running)")
                           .arg(node.objectName());
        return false;
    }
    return GtIntelliGraphExecutor::canEvaluateNode(node, outIdx);

}

GtIntelliGraphParallelExecutor::~GtIntelliGraphParallelExecutor()
{
    if (!GtIntelliGraphParallelExecutor::isReady())
    {
        gtWarning().verbose() << __func__ << "is not ready for deletion!";
    }
}

void
GtIntelliGraphParallelExecutor::onFinished()
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
GtIntelliGraphParallelExecutor::onCanceled()
{
    gtWarning().verbose() << __func__ << m_node;
}

void
GtIntelliGraphParallelExecutor::onResultReady(int idx)
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

    std::vector<NodeData> outData = m_watcher.resultAt(idx);

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

    if (m_port != gt::ig::invalid<PortIndex>())
    {
        return emitOutDataUpdated(m_port);
    }
    for (PortIndex outIdx{0}; outIdx < p.outPorts.size(); ++outIdx)
    {
        emitOutDataUpdated(outIdx);
    }
}

bool
GtIntelliGraphParallelExecutor::evaluateNode(GtIntelliGraphNode& node)
{
    m_port = gt::ig::invalid<PortIndex>();
    return evaluateNodeHelper(node);
}

bool
GtIntelliGraphParallelExecutor::evaluatePort(GtIntelliGraphNode& node, PortIndex idx)
{
    m_port = idx;
    return evaluateNodeHelper(node);
}

bool
GtIntelliGraphParallelExecutor::evaluateNodeHelper(GtIntelliGraphNode& node)
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
                this]() -> std::vector<NodeData>
    {
        auto clone = gt::unique_qobject_cast<GtIntelliGraphNode>(
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
        if (targetPort != gt::ig::invalid<PortIndex>())
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
GtIntelliGraphParallelExecutor::isReady() const
{
    return m_watcher.isCanceled() || m_watcher.isFinished();
}
