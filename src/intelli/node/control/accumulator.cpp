/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#include "intelli/node/control/accumulator.h"

#include "intelli/graphdatamodel.h"
#include "intelli/graphexecutor.h"
#include "intelli/data/list.h"
#include "intelli/data/double.h"
#include "intelli/nodedatafactory.h"
#include "intelli/private/utils.h"

#include <gt_utilities.h>
#include <gt_eventloop.h>

using namespace intelli;

constexpr const char* C_NAME_IN_NODE = "Input";
constexpr const char* C_NAME_OUT_NODE = "Output";
constexpr const char* C_NAME_LAST_ITER_NODE = "Last Iteration";
constexpr const char* C_NAME_INDEX_NODE = "Index";

namespace
{

static void onPortInserted(AccumulatorGraphNode* root,
                           AbstractGraphProvider* provider,
                           PortType type,
                           PortIndex idx,
                           bool invert = false)
{
    if (type != PortType::In)
    {
        gtDebug() << QObject::tr("ACC PORT INSERTING (SKIPPING)") << *root->port(root->portId(type, idx));
        return;
    }

    assert(root);
    assert(root->isDynamicPort(type, idx));
    assert(provider);

    auto const makeError = [root, type, idx](){
        return relativeNodePath(*root) + QStringLiteral(": ") +
               QObject::tr("Failed to add %3put port (%1/%2)!")
                   .arg(toString(idx),
                        toString(type),
                        type == PortType::In ? "in":"out");
    };

    PortInfo* srcPort = root->port(root->portId(type, idx));

    gtDebug() << QObject::tr("ACC PORT INSERTING") << *srcPort << idx;

    if (!srcPort)
    {
        gtError() << makeError() << QObject::tr("(Source port not found)");
        return;
    }

    if (!provider)
    {
        gtError() << makeError() << QObject::tr("(provider not found)");
        return;
    }

    NodePort copy = *srcPort;
    if (idx == 0)
    {
        copy.typeId = NodeDataFactory::instance().innerType(copy.typeId);
    }

    PortId addedPortId = provider->addPort(copy);
    if (!addedPortId.isValid())
    {
        gtError() << makeError()
                  << QObject::tr("(Adding port to provider failed)");
        return;
    }
    assert(addedPortId == srcPort->id());
}

static void onPortChanged(AccumulatorGraphNode* root,
                          AbstractGraphProvider* provider,
                          PortId portId)
{
    assert(portId.isValid());
    assert(root);
    assert(provider);

    PortType type = root->portType(portId);
    assert(type != PortType::NoType);
    assert(root->isDynamicPort(type, root->portIndex(type, portId)));

    if (type != PortType::In) return;

    auto const makeError = [root, type, portId](){
        return relativeNodePath(*root) + QStringLiteral(": ") +
               QObject::tr("Failed to update %2put port (%1)!")
                   .arg(toString(portId),
                        type == PortType::In ? "in":"out");
    };

    PortInfo* srcPort = root->port(portId);
    if (!srcPort)
    {
        gtError() << makeError() << QObject::tr("(Source port not found)");
        return;
    }

    if (!provider)
    {
        gtError() << makeError() << QObject::tr("(provider not found)");
        return;
    }

    PortInfo* port = provider->port(srcPort->id());
    if (!port)
    {
        gtError() << makeError()
                  << QObject::tr("(Updating port of provider failed)");
        return;
    }

    NodePort copy = *srcPort;
    if (provider->portIndex(invert(provider->providerType()), portId) == 0)
    {
        copy.typeId = NodeDataFactory::instance().innerType(copy.typeId);
    }
    port->assign(copy);
    emit provider->portChanged(port->id());
}

static void onPortDeleted(AccumulatorGraphNode* root,
                          AbstractGraphProvider* provider,
                          PortType type,
                          PortIndex idx)
{
    if (type != PortType::In) return;

    assert(root);
    assert(root->isDynamicPort(type, idx));
    assert(provider);

    auto const makeError = [root, type, idx](){
        return relativeNodePath(*root) + QStringLiteral(": ") +
               QObject::tr("Failed to delete %3put port (%1/%2)!")
                   .arg(toString(idx),
                        toString(type),
                        type == PortType::In ? "in":"out");
    };

    auto portId = root->portId(type, idx);
    if (!portId.isValid())
    {
        gtError() << makeError() << QObject::tr("(Source port not found)");
        return;
    }

    if (!provider)
    {
        gtError() << makeError() << QObject::tr("(provider not found)");
        return;
    }

    if (!provider->removePort(portId))
    {
        gtError() << makeError()
                  << QObject::tr("(Removing port of provider failed)");
        return;
    }
}

} // namespace

AccumulatorGraphNode::AccumulatorGraphNode() :
    Graph(QStringLiteral("Accumulator"), false)
{
    setNodeEvalMode(NodeEvalMode::Blocking);

    NodeId nextId{0};
    Position offset{0, 100};

    auto input = std::make_unique<GraphInputProvider>();
    input->setCaption(C_NAME_IN_NODE);
    input->setPos(input->pos());
    input->setDefault(true);
    input->setId(nextId++);

    connect(this, &Node::portInserted,
            input.get(), [this, node = input.get()](PortType type, PortIndex idx){
        ::onPortInserted(this, node, type, idx);
    }, Qt::DirectConnection);
    connect(this, &Node::portChanged,
            input.get(), [this, node = input.get()](PortId portId){
        ::onPortChanged(this, node, portId);
    }, Qt::DirectConnection);
    connect(this, &Node::portAboutToBeDeleted,
            input.get(), [this, node = input.get()](PortType type, PortIndex idx){
        ::onPortDeleted(this, node, (type), idx);
    }, Qt::DirectConnection);

    auto output = std::make_unique<GraphOutputProvider>();
    output->setCaption(C_NAME_OUT_NODE);
    output->setPos(output->pos());
    output->setDefault(true);
    output->setId(nextId++);
    synchronizePorts(*output);

    auto lastIter = std::make_unique<AccumulatorLastIterationProvider>();
    lastIter->setCaption(C_NAME_LAST_ITER_NODE);
    lastIter->setPos(lastIter->pos() + offset);
    lastIter->setDefault(true);
    lastIter->setId(nextId++);
    synchronizePorts(*output, *lastIter);

    auto indexNode = std::make_unique<GraphInputProvider>();
    indexNode->setCaption(C_NAME_INDEX_NODE);
    indexNode->setPos(indexNode->pos() + (2 * offset));
    indexNode->setDefault(true);
    indexNode->setId(nextId++);
    m_index = indexNode->addPort(makePort(typeId<IntData>()).setCaption("index"));

    appendNode(std::move(input), NodeIdPolicy::Keep);
    appendNode(std::move(output), NodeIdPolicy::Keep);
    appendNode(std::move(lastIter), NodeIdPolicy::Keep);
    appendNode(std::move(indexNode), NodeIdPolicy::Keep);

    m_listIn = addInPort(makePort(typeId<list<DoubleData>>()));
    m_out = addOutPort(makePort(typeId<DoubleData>()));
}

void
AccumulatorGraphNode::eval()
{
    auto makeError = [this](){
        return gt::quoted(relativeNodePath(*this), "[", "] ") +
               tr("evaluation failed!");
    };

    if (!port(m_out))
    {
        gtError() << makeError() << tr("invalid output port!");
        return evalFailed();
    }

    auto* dataModel = qobject_cast<GraphDataModel*>(exec::nodeDataInterface(*this));
    if (!dataModel)
    {
        gtError() << makeError() << tr("data model not found!");
        return evalFailed();
    }

    if (!setNodeData(m_out, nullptr))
    {
        gtError() << makeError();
        return evalFailed();
    }

    // setup
    auto listData = qobject_cast<BaseListData const*>(nodeData(m_listIn).get());
    if (!listData || !port(m_listIn))
    {
        gtError() << makeError() << tr("invalid list data!");
        return evalFailed();
    }

    auto* inputNode = findDirectChild<GraphInputProvider*>(C_NAME_IN_NODE);
    auto* lastIterNode = findDirectChild<GraphInputProvider*>(C_NAME_LAST_ITER_NODE);
    auto* outputNode = findDirectChild<GraphOutputProvider*>(C_NAME_OUT_NODE);
    auto* indexNode = findDirectChild<GraphInputProvider*>(C_NAME_INDEX_NODE);

    if (!inputNode || !outputNode || !lastIterNode || !indexNode)
    {
        gtError() << makeError() << tr("input/ouput providers not found!");
        return evalFailed();
    }

    gtDebug() << "RESSETING DATA";
    if (!dataModel->setNodeData(lastIterNode->uuid(), m_out, nullptr))
    {
        gtError() << makeError()
                  << tr("failed to reset last iter data for port '%1'!");
        return evalFailed();
    }

    // set input data
    gtDebug() << "SETTING INPUT DATA";
    for (NodePort const& port : ports(PortType::In))
    {
        if (port.id() == m_listIn) continue;

        if (!inputNode->port(port.id()))
        {
            gtError() << makeError()
                      << tr("port '%1' in input provider not found!")
                             .arg(toString(port));
            return evalFailed();
        }

        gtDebug() << "->" << port;
        if (!dataModel->setNodeData(inputNode->uuid(), port.id(), nodeData(port.id())))
        {
            gtError() << makeError()
                      << tr("failed to set input data for port '%1'!")
                             .arg(toString(port));
            return evalFailed();
        }
    }

    int index = {0};
    for (NodeDataPtr const& current : listData->iterate())
    {
        gtDebug() << "ITERATION" << index;
        // set input data
        if (!inputNode->port(m_listIn))
        {
            gtError() << makeError()
                      << tr("port '%1' in input provider not found!")
                             .arg(toString(m_listIn));
            return evalFailed();
        }

        gtDebug() << "-> setting:" << current;
        if (!dataModel->setNodeData(inputNode->uuid(), m_listIn, current))
        {
            gtError() << makeError()
                      << tr("failed to set input data for port '%1'!")
                             .arg(toString(m_listIn));
            return evalFailed();
        }

        // set index data
        gtDebug() << "-> index:" << index;
        if (!dataModel->setNodeData(indexNode->uuid(), m_index, makeNodeData<IntData>(index++)))
        {
            gtError() << makeError()
                      << tr("failed to set index data!");
            return evalFailed();
        }

        GtEventLoop loop{std::chrono::seconds{60}};

        // evaluate branch
        GraphExecutor executor{*this, *dataModel};

        loop.connectSuccess(&executor, &GraphExecutor::targetNodesEvaluated);
        loop.connectAbort(this, &Graph::graphAboutToBeDeleted);

        auto future = executor.evaluateNode(outputNode->id());

        // TODO: cannot block main thread here!
        auto status = loop.exec();
        if (status != GtEventLoop::Success)
        {
            gtDebug() << "FAILED";
            return evalFailed();
        }

        gtDebug() << "SETTING NEXT ITERATION DATA";
        // set output data
        for (NodePort const& port : ports(PortType::Out))
        {
            if (!lastIterNode->port(port.id()))
            {
                gtError() << makeError()
                          << tr("port '%1' in last iter provider not found!")
                                 .arg(toString(port));
                return evalFailed();
            }

            gtDebug() << "->" << port << dataModel->nodeData(outputNode->uuid(), port.id()).ptr;
            if (!dataModel->setNodeData(lastIterNode->uuid(), port.id(), dataModel->nodeData(outputNode->uuid(), port.id())))
            {
                gtError() << makeError()
                          << tr("failed to set last iter data for port '%1'!")
                                 .arg(toString(port));
                return evalFailed();
            }
        }
    }

    gtDebug() << "SETTING OUTPUT DATA";
    // set output
    for (NodePort const& port : ports(PortType::Out))
    {
        if (!outputNode->port(port.id()))
        {
            gtError() << makeError()
                      << tr("port '%1' in output provider not found!")
                             .arg(toString(port));
            return evalFailed();
        }

        gtDebug() << "->" << port << dataModel->nodeData(lastIterNode->uuid(), port.id());
        if (!setNodeData(port.id(), dataModel->nodeData(lastIterNode->uuid(), port.id())))
        {
            gtError() << makeError()
                      << tr("failed to set output data for port '%1'!")
                             .arg(toString(port));
            return evalFailed();
        }
    }
}

