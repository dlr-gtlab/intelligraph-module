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

    connect(&m_watcher, &Watcher::started,
            this, &GtIntelliGraphParallelExecutor::onStarted);
    connect(&m_watcher, &Watcher::finished,
            this, &GtIntelliGraphParallelExecutor::onFinished);
    connect(&m_watcher, &Watcher::canceled,
            this, &GtIntelliGraphParallelExecutor::onCanceled);
    connect(&m_watcher, &Watcher::resultReadyAt,
            this, &GtIntelliGraphParallelExecutor::onResultReady);

//    connect(&m_watcher, &Watcher::resultsReadyAt,
//            this, &GtIntelliGraphParallelExecutor::onResultsReady);
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
    return GtIntellIGraphExecutor::canEvaluateNode(node, outIdx);

}

GtIntelliGraphParallelExecutor::~GtIntelliGraphParallelExecutor()
{
    gtTrace().verbose() << __func__ << "is ready for deletion?:" << isReady();
}

void
GtIntelliGraphParallelExecutor::onStarted()
{
    gtTrace().verbose() << std::left << std::setw(15) << __func__;
}

void
GtIntelliGraphParallelExecutor::onFinished()
{
    gtTrace().verbose() << std::left << std::setw(15) << __func__ << m_node << m_watcher.isRunning();

    if (!m_node)
    {
        gtError() << QObject::tr("Cannot finish transfer of node data! "
                                 "(Invalid node)");
        return;
    }

//    m_node.clear();
    m_collected = true;

    if (m_node)
    {
        auto& pimpl = accessImpl(*m_node);
        if (pimpl.requiresEvaluation)
        {
            gtDebug() << "reavluating node!" << m_node;
            m_node->updateNode();
        }
    }
}

void
GtIntelliGraphParallelExecutor::onCanceled()
{
    gtWarning().verbose() << std::left << std::setw(15) << __func__ << m_node;
}

void
GtIntelliGraphParallelExecutor::onResultReady(int idx)
{
    gtTrace().verbose() << std::left << std::setw(15) << __func__ << m_node;

    if (!m_node)
    {
        gtError() << tr("Cannot transfer node data! (Invalid node)");
        return;
    }

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

    for (PortIndex outIdx{0}; outIdx < p.outPorts.size(); ++outIdx)
    {
        auto& out = p.outData.at(outIdx);

        emit m_node->evaluated(outIdx);

        out ? emit m_node->outDataUpdated(outIdx) :
              emit m_node->outDataInvalidated(outIdx);
    }

    m_collected = true;
}

void
GtIntelliGraphParallelExecutor::onResultsReady(int begin, int end)
{
    gtTrace().verbose() << std::left << std::setw(15) << __func__ << begin << end;
}

bool
GtIntelliGraphParallelExecutor::evaluateNode(GtIntelliGraphNode& node)
{
    if (!canEvaluateNode(node)) return false;

    m_node = &node;
    m_collected = false;

    auto& p = accessImpl(node);

    auto run = [inData = p.inData,
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

        if (auto* graph = qobject_cast<GtIntelliGraph*>(clone.get()))
        {
            graph->initGroupProviders();
            graph->makeModelAdapter(gt::ig::DummyModel);
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
GtIntelliGraphParallelExecutor::evaluatePort(GtIntelliGraphNode& node, PortIndex idx)
{
    return evaluateNode(node);
}

bool
GtIntelliGraphParallelExecutor::isReady() const
{
    return m_watcher.isFinished();
}
