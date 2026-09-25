
#include "intelli/node/control/mapreduce.h"
#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"
#include "intelli/data/list.h"
#include "intelli/data/double.h"
#include "intelli/nodedatafactory.h"
#include "intelli/graphdatamodel.h"
#include "intelli/graphexecutor.h"
#include "intelli/graphdatamodel.h"
#include "intelli/graphexecutor.h"
#include "intelli/private/utils.h"

#include <gt_utilities.h>
#include <gt_eventloop.h>

using namespace intelli;

constexpr const char* C_NAME_IN_NODE = "Input";
constexpr const char* C_NAME_OUT_NODE = "Output";
constexpr const char* C_NAME_INDEX_NODE = "Index";

namespace
{

static void onPortInserted(MapReduceGroupNode* root,
                           AbstractGraphProvider* provider,
                           PortType type,
                           PortIndex idx,
                           bool invert = false)
{
    if (type != PortType::In) return;

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

static void onPortChanged(MapReduceGroupNode* root,
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

static void onPortDeleted(MapReduceGroupNode* root,
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

MapReduceGroupNode::MapReduceGroupNode() :
    Graph(QStringLiteral("Map Redeuce"), false),
    m_operation("reduceOperation", tr("Reduce Operation"), tr("Reduce Operation"))
{
    setNodeEvalMode(NodeEvalMode::Blocking);

    registerProperty(m_operation);

    NodeId nextId{0};
    Position offset{0, 100};

    auto input = std::make_unique<GraphInputProvider>();
    input->setCaption(C_NAME_IN_NODE);
    input->setPos(input->pos() - offset);
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

    auto indexNode = std::make_unique<GraphInputProvider>();
    indexNode->setCaption(C_NAME_INDEX_NODE);
    indexNode->setPos(indexNode->pos() + offset);
    indexNode->setDefault(true);
    indexNode->setId(nextId++);
    m_index = indexNode->addPort(makePort(typeId<IntData>()).setCaption("index"));

    appendNode(std::move(input), NodeIdPolicy::Keep);
    appendNode(std::move(output), NodeIdPolicy::Keep);
    appendNode(std::move(indexNode), NodeIdPolicy::Keep);

    m_listIn = addInPort(makePort(typeId<list<DoubleData>>()));
    m_out = addOutPort(makePort(typeId<DoubleData>()));
}

void
MapReduceGroupNode::eval()
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
    auto* outputNode = findDirectChild<GraphOutputProvider*>(C_NAME_OUT_NODE);
    auto* indexNode = findDirectChild<GraphInputProvider*>(C_NAME_INDEX_NODE);

    if (!inputNode || !outputNode || !outputNode || !indexNode)
    {
        gtError() << makeError() << tr("input/ouput providers not found!");
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

    QVector<NodeDataPtr> accumulated;
    accumulated.reserve(listData->iterate().size());

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

        gtDebug() << "NEXT";
        // set output
        for (NodePort const& port : { *port(m_out) })
        {
            if (!outputNode->port(port.id()))
            {
                gtError() << makeError()
                          << tr("port '%1' in output provider not found!")
                                 .arg(toString(port));
                return evalFailed();
            }

            accumulated.append(dataModel->nodeData(outputNode->uuid(), port.id()).ptr);
        }
    }

    gtDebug() << "REDUCING";

    NodeDataPtr outputData;

    switch (m_operation)
    {
    case ReduceAdd:
    {
        QVector<double> tmp;
        std::transform(accumulated.begin(),
                       accumulated.end(),
                       std::back_inserter(tmp), [](NodeDataPtr const& data){
                           auto converted = convert<DoubleData>(data);
                           return converted ? converted->value() : 0.0;
                       });
        double sum = std::accumulate(tmp.begin(), tmp.end(), 0.0, std::plus<double>{});
        outputData = makeNodeData<DoubleData>(sum);
        break;
    }
    case ReduceSubstract:
    {
        QVector<double> tmp;
        std::transform(accumulated.begin(),
                       accumulated.end(),
                       std::back_inserter(tmp), [](NodeDataPtr const& data){
                           auto converted = convert<DoubleData>(data);
                           return converted ? converted->value() : 0.0;
                       });
        double sum = std::accumulate(tmp.begin(), tmp.end(), 0.0, std::minus<double>{});
        outputData = makeNodeData<DoubleData>(sum);
        break;
    }
    case ReduceMultiply:
    {
        QVector<double> tmp;
        std::transform(accumulated.begin(),
                       accumulated.end(),
                       std::back_inserter(tmp), [](NodeDataPtr const& data){
                           auto converted = convert<DoubleData>(data);
                           return converted ? converted->value() : 0.0;
                       });
        double sum = std::accumulate(tmp.begin(), tmp.end(), 0.0, std::multiplies<double>{});
        outputData = makeNodeData<DoubleData>(sum);
        break;
    }
    case ReduceMax:
    {
        QVector<double> tmp;
        std::transform(accumulated.begin(),
                       accumulated.end(),
                       std::back_inserter(tmp), [](NodeDataPtr const& data){
                           auto converted = convert<DoubleData>(data);
                           return converted ? converted->value() : std::numeric_limits<double>::min();
                       });
        auto max = std::max_element(tmp.begin(), tmp.end());
        if (max == tmp.end() || *max == std::numeric_limits<double>::min()) outputData = nullptr;
        else outputData = makeNodeData<DoubleData>(*max);
        break;
    }
    case ReduceMin:
    {
        QVector<double> tmp;
        std::transform(accumulated.begin(),
                       accumulated.end(),
                       std::back_inserter(tmp), [](NodeDataPtr const& data){
                           auto converted = convert<DoubleData>(data);
                           return converted ? converted->value() : std::numeric_limits<double>::max();
                       });
        auto min = std::min_element(tmp.begin(), tmp.end());
        if (min == tmp.end() || *min == std::numeric_limits<double>::max()) outputData = nullptr;
        else outputData = makeNodeData<DoubleData>(*min);
        break;
    }
    }

    gtDebug() << "SETTING OUTPUT DATA" << outputData;
    // set output
    for (NodePort const& port : { *port(m_out) })
    {
        if (!outputNode->port(port.id()))
        {
            gtError() << makeError()
                      << tr("port '%1' in output provider not found!")
                             .arg(toString(port));
            return evalFailed();
        }

        if (!setNodeData(port.id(), outputData))
        {
            gtError() << makeError()
                      << tr("failed to set output data for port '%1'!")
                             .arg(toString(port));
            return evalFailed();
        }
    }
}

