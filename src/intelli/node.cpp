/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/node.h"

#include "intelli/nodedatafactory.h"
#include "intelli/nodedatainterface.h"
#include "intelli/exec/detachedexecutor.h"
#include "intelli/private/node_impl.h"
#include "intelli/private/utils.h"

#include <gt_qtutilities.h>
#include <gt_coreapplication.h>

#include <QRegExpValidator>
#include <QVBoxLayout>

using namespace intelli;

NodeDataPtr
intelli::convert(NodeDataPtr const& data, TypeId const& to)
{
    // forwarding call
    return NodeDataFactory::instance().convert(data, to);
}

std::unique_ptr<QWidget>
intelli::makeBaseWidget()
{
    auto base = std::make_unique<QWidget>();
    auto* layout = new QVBoxLayout(base.get());
    layout->setContentsMargins(0, 0, 0, 0);
    return base;
}

Node::Node(QString const& modelName, GtObject* parent) :
    GtObject(parent),
    pimpl(std::make_unique<Impl>(modelName))
{
    setFlag(UserDeletable, true);
    // nodes can be renamed using custom rename action
    setFlag(UserRenamable, false);

    static const QString catData = QStringLiteral("Node-Data");
    registerProperty(pimpl->id, catData);
    registerProperty(pimpl->posX, catData);
    registerProperty(pimpl->posY, catData);
    registerProperty(pimpl->sizeWidth, catData);
    registerProperty(pimpl->sizeHeight, catData);

    static const QString catEval = QStringLiteral("Node-Evaluation");
    registerProperty(pimpl->isActive, catEval);

    pimpl->id.setReadOnly(true);
    pimpl->posX.setReadOnly(true);
    pimpl->posY.setReadOnly(true);
    pimpl->sizeWidth.setReadOnly(true);
    pimpl->sizeHeight.setReadOnly(true);

#ifndef GT_INTELLI_DEBUG_NODE_PROPERTIES
    bool hide = !gtApp || !gtApp->devMode();
    pimpl->id.hide(hide);
    pimpl->posX.hide(true);
    pimpl->posY.hide(true);
    pimpl->sizeWidth.hide(true);
    pimpl->sizeHeight.hide(true);
#endif

    setCaption(modelName);
    
    connect(this, &Node::portInserted,
            this, &Node::nodeChanged);
    connect(this, &Node::portDeleted,
            this, &Node::nodeChanged);
    connect(this, &QObject::objectNameChanged,
            this, &Node::nodeChanged);

    connect(&pimpl->isActive, &GtAbstractProperty::changed, this, [this](){
        emit isActiveChanged();
        if (pimpl->isActive) emit triggerNodeEvaluation();
    });

    // must subscribe to both properties incase only one changes
    connect(&pimpl->posX, &GtAbstractProperty::changed, this, [this](){
        emit nodePositionChanged();
    });
    connect(&pimpl->posY, &GtAbstractProperty::changed, this, [this](){
        emit nodePositionChanged();
    });

    // must subscribe to both properties incase only one changes
    connect(&pimpl->sizeWidth, &GtAbstractProperty::changed, this, [this](){
        emit nodeSizeChanged();
    });
    connect(&pimpl->sizeHeight, &GtAbstractProperty::changed, this, [this](){
        emit nodeSizeChanged();
    });

    connect(this, &Node::portConnected, this, [this](PortId portId){
        auto* port = this->port(portId);
        if (port) port->m_isConnected = true;
    }, Qt::DirectConnection);

    connect(this, &Node::portDisconnected, this, [this](PortId portId){
        auto* port = this->port(portId);
        if (port) port->m_isConnected = false;
        }, Qt::DirectConnection);
}

Node::~Node()
{
    emit nodeAboutToBeDeleted(id());
}

void
Node::setActive(bool active)
{
    if (pimpl->isActive == active) return;
    pimpl->isActive = active;
}

bool
Node::isActive() const
{
    return pimpl->isActive;
}

void
Node::setId(NodeId id)
{
    pimpl->id = id;
}

NodeId
Node::id() const
{
    return NodeId{pimpl->id};
}

Node&
Node::setPos(Position pos)
{
    if (this->pos() != pos)
    {
        pimpl->posX = pos.x();
        pimpl->posY = pos.y();
        changed();
    }
    return *this;
}

Position
Node::pos() const
{
    return { pimpl->posX, pimpl->posY };
}

void
Node::setSize(QSize size)
{
    if (nodeFlags() & ResizableHOnly) size.setHeight(0);

    if (this->size() != size)
    {
        pimpl->sizeWidth = size.width();
        pimpl->sizeHeight = size.height();
        changed();
    }
}

QSize
Node::size() const
{
    if (nodeFlags() & ResizableHOnly) return { pimpl->sizeWidth, 0 };

    return { pimpl->sizeWidth, pimpl->sizeHeight };
}

QSize
Node::size(QSize fallback) const
{
    auto size = this->size();

    if (size.width() <= 0) size.setWidth(fallback.width());
    if (size.height() <= 0) size.setHeight(fallback.height());

    return size;
}

void
Node::updateObjectName()
{
    gt::setUniqueName(*this, baseObjectName());
}

bool
Node::isValid() const
{
    return id() != invalid<NodeId>();
}

void
Node::setNodeFlag(NodeFlag flag, bool enable)
{
    switch (flag)
    {
    case Deprecated:
        if (toolTip().isEmpty())
        {
            setToolTip(tr("This node is deprecated and will "
                          "be removed in a future relase."));
        }
        break;
    default:
        break;
    }

    enable ? pimpl->flags |= flag :
             pimpl->flags &= ~flag;
}

void
Node::setNodeEvalMode(NodeEvalMode mode)
{
    pimpl->evalMode = mode;
}

NodeFlags
Node::nodeFlags() const
{
    return pimpl->flags;
}

NodeEvalMode
Node::nodeEvalMode() const
{
    return pimpl->evalMode;
}

void
Node::setToolTip(QString const& tooltip)
{
    pimpl->toolTip = tooltip;
}

QString const&
Node::tooltip() const
{
    return pimpl->toolTip;
}

QString const&
Node::toolTip() const
{
    return pimpl->toolTip;
}

Node&
Node::setCaption(QString const& caption)
{
    gt::setUniqueName(*this, caption);
    return *this;
}

QString
Node::caption() const
{
    return objectName();
}

QString
Node::baseObjectName() const
{
    static QRegularExpression const regExp(QStringLiteral(R"((.+)(\s?\[\d+\]))"));
    constexpr int groupNo = 1;

    auto const& name = objectName();
    auto const& match = regExp.match(name);
    return (match.capturedLength(groupNo) > 0) ? match.captured(groupNo) : name;
}

QString const&
Node::modelName() const
{
    return pimpl->modelName;
}

std::vector<Node::PortInfo> const&
Node::ports(PortType type) const noexcept(false)
{
    return pimpl->ports(type);
}

PortId
Node::addInPort(PortInfo port, PortPolicy policy) noexcept(false)
{
    return insertInPort(std::move(port), -1, policy);
}

PortId
Node::addOutPort(PortInfo port) noexcept(false)
{
    return insertOutPort(std::move(port), -1);
}

PortId
Node::insertInPort(PortInfo port, int idx, PortPolicy policy) noexcept(false)
{
    port.optional = policy == PortPolicy::Optional;
    return insertPort(PortType::In, std::move(port), idx);
}

PortId
Node::insertOutPort(PortInfo port, int idx) noexcept(false)
{
    return insertPort(PortType::Out, std::move(port), idx);
}

PortId
Node::insertPort(PortType type, PortInfo port, int idx) noexcept(false)
{
    auto const makeError = [this, type, idx](){
        return objectName() + QStringLiteral(": ") +
               tr("Failed to insert port at idx %1 (%2)").arg(idx).arg(toString(type));
    };

    if (port.typeId.isEmpty())
    {
        gtWarning() << makeError() << tr("(Invalid typeId specified)");
        return PortId{};
    }

    // reset is connected flag
    port.m_isConnected = false;

    auto& ports = pimpl->ports(type);
    auto iter = ports.end();

    if (idx >= 0 && static_cast<size_t>(idx) < ports.size())
    {
        iter = std::next(ports.begin(), idx);
    }

    // update port id if necessary
    PortId id = pimpl->incNextPortId(port.m_id);
    if (id == invalid<PortId>())
    {
        gtWarning() << makeError() << tr("(Port id %1 already exists)").arg(port.m_id);
        return PortId{};
    }

    port.m_id = id;

    // notify model
    PortIndex pidx = PortIndex::fromValue(std::distance(ports.begin(), iter));
    emit portAboutToBeInserted(type, pidx);
    auto finally = gt::finally([=](){ emit portInserted(type, pidx); });

    // do the insertion
    ports.insert(iter, std::move(port));

    return id;
}

bool
Node::removePort(PortId id)
{
    auto port = pimpl->findPort(id);
    if (!port) return false;

    // notify model
    emit portAboutToBeDeleted(port.type, port.idx);
    auto finally = gt::finally([type = port.type, idx = port.idx, this](){
        emit portDeleted(type, idx);
    });

    port.ports->erase(std::next(port.ports->begin(), port.idx));

    return true;
}

NodeEvalState
Node::nodeEvalState() const
{
    NodeDataInterface* model = pimpl->dataInterface;
    if (!model)
    {
        return NodeEvalState::Invalid;
    }
    return model->nodeEvalState(this->uuid());
}

Node::NodeDataPtr
Node::nodeData(PortId id) const
{
    NodeDataInterface* model = pimpl->dataInterface;
    if (!model)
    {
        gtWarning()
            << utils::logId(*this)
            << tr("Failed to access node data, data interface not found!");
        return {};
    }

    return model->nodeData(this->uuid(), id);
}

bool
Node::setNodeData(PortId id, NodeDataPtr data)
{
    NodeDataInterface* model = pimpl->dataInterface;
    if (!model)
    {
        gtWarning()
            << utils::logId(*this)
            << tr("Failed to set node data, data interface not found!");
        return false;
    }

    return model->setNodeData(this->uuid(), id, std::move(data));
}

Node::PortInfo*
Node::port(PortId id) noexcept
{
    for (auto* ports : { &pimpl->inPorts, &pimpl->outPorts })
    {
        auto iter = Impl::find(*ports, id);

        if (iter != ports->end()) return &(*iter);
    }

    return nullptr;
}

Node::PortInfo const*
Node::port(PortId id) const noexcept
{
    return const_cast<Node*>(this)->port(id);
}

PortIndex
Node::portIndex(PortType type, PortId id) const noexcept(false)
{
    auto& ports = this->ports(type);

    auto iter = Impl::find(ports, id);

    if (iter != ports.end())
    {
        return PortIndex::fromValue(std::distance(ports.begin(), iter));
    }

    return PortIndex{};
}

Node::PortType
Node::portType(PortId id) const noexcept(false)
{
    auto port = pimpl->findPort(id);
    if (!port) return PortType::NoType;

    return port.type;
}

PortId
Node::portId(PortType type, PortIndex idx) const noexcept(false)
{
    auto& ports = this->ports(type);

    if (idx >= ports.size()) return PortId{};

    return ports.at(idx).m_id;
}

bool
Node::isPortConnected(PortId portId) const
{
    auto* port = this->port(portId);
    return port && port->isConnected();
}

GraphUserVariables const*
Node::userVariables() const
{
    NodeDataInterface* model = pimpl->dataInterface;
    if (!model)
    {
        return findDirectChild<GraphUserVariables const*>();
    }

    return model->userVariables();
}

void
Node::nodeEvent(NodeEvent const* event)
{
    Q_UNUSED(event);
    // nothing to do here
}

void
Node::eval()
{
    // nothing to do here
}

void
Node::evalFailed()
{
    NodeDataInterface* model = pimpl->dataInterface;
    if (!model)
    {
        gtWarning()
            << utils::logId(*this)
            << tr("Failed to set node evaluation status, data interface not found!");
        return;
    }

    model->setNodeEvaluationFailed(uuid());
}

void
Node::registerWidgetFactory(WidgetFactory factory)
{
    pimpl->widgetFactory = std::move(factory);
}

void
Node::registerWidgetFactory(WidgetFactoryNoArgs factory)
{
    registerWidgetFactory([f = std::move(factory)](Node&){
        return f();
    });
}

//////////////////////////////////////////////////////

/**
 * @brief The INode class. Helper class to access private or protected
 * members of a node outside the `Node` class in a controlled manner.
 */
class intelli::INode
{
    INode() = delete;

public:

    inline static void
    evaluateNode(Node& node)
    {
        node.eval();
    }

    inline static void
    setNodeDataInterface(Node& node, NodeDataInterface* interface)
    {
        bool hadValue = (node.pimpl->dataInterface);
        node.pimpl->dataInterface = interface;

        if (!hadValue)
        {
            Node::NodeEvent e{
                Node::DataInterfaceAvailableEvent
            };
            node.nodeEvent(&e);
        }
    }

    GT_NO_DISCARD
    inline static NodeDataInterface*
    // cppcheck-suppress constParameter
    nodeDataInterface(Node& node)
    {
        return node.pimpl->dataInterface;
    }

    GT_NO_DISCARD
    inline static bool
    triggerNodeEvaluation(Node& node)
    {
        auto evalMode = node.nodeEvalMode();
        if (evalMode == NodeEvalMode::NoEvaluationRequired) return true;

        size_t evalFlag = (size_t)node.nodeEvalMode();
        if (evalFlag & IsDetachedMask)
        {
            return exec::detachedEvaluation(node);
        }
        if (evalFlag & IsBlockingMask)
        {
            return exec::blockingEvaluation(node);
        }

        gtError() << utils::logId(node)
                  << QObject::tr("Unhandled eval mode! (%1)").arg((size_t)evalFlag);

        return false;
    }
};

namespace
{

/// conditionally updates node data interface if model is not null
GT_NO_DISCARD
inline static NodeDataInterface*
updateNodeDataInterface(Node& node, NodeDataInterface* model)
{
    if (model)
    {
        INode::setNodeDataInterface(node, model);
        assert(INode::nodeDataInterface(node) == model);
        return model;
    }

    model = INode::nodeDataInterface(node);
    if (!model)
    {
        gtError() << utils::logId(node)
                  << QObject::tr("Failed to evaluate node! (Missing data interface)");
        return nullptr;
    }
    return model;
}

} // namespace

bool
intelli::exec::detachedEvaluation(Node& node, NodeDataInterface* model)
{
    if (!::updateNodeDataInterface(node, model)) return false;

    auto executor = node.findChild<DetachedExecutor*>();
    if (executor && !executor->canEvaluateNode())
    {
        gtError() << utils::logId(node)
                  << QObject::tr("Failed to evaluate node! (Node is already executing)");
        return false;
    }

    if (!executor) executor = new DetachedExecutor(&node);

    if (!executor->evaluateNode(node))
    {
        delete executor;
        return false;
    }

    return true;
}

bool
intelli::exec::blockingEvaluation(Node& node, NodeDataInterface* model)
{
    model = ::updateNodeDataInterface(node, model);
    if (!model) return false;

    auto cmd = model->nodeEvaluation(node.uuid());
    Q_UNUSED(cmd);

    // cleanup routine
    auto finally = gt::finally([&node](){
        emit node.computingFinished();
    });
    Q_UNUSED(finally);

    emit node.computingStarted();

    INode::evaluateNode(node);

    return true;
}

bool
intelli::exec::triggerNodeEvaluation(Node& node, NodeDataInterface* model)
{
    if (!::updateNodeDataInterface(node, model)) return false;

    return INode::triggerNodeEvaluation(node);
}

void
intelli::exec::setNodeDataInterface(Node& node, NodeDataInterface* model)
{
    return INode::setNodeDataInterface(node, model);
}

NodeDataInterface*
intelli::exec::nodeDataInterface(Node& node)
{
    return INode::nodeDataInterface(node);
}
