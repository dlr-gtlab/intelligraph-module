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

#if GT_VERSION >= 0x020100
    setFlag(UserRenamable, true);
#else
    setFlag(UserRenamable, false);
#endif

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

    // position is changed in pairs -> sufficient to subscribe to changes
    // to y-pos (avoids emitting singal twice)
    connect(&pimpl->posY, &GtAbstractProperty::changed, this, [this](){
        emit nodePositionChanged();
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
    return { pimpl->sizeWidth, pimpl->sizeHeight };
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
        if (tooltip().isEmpty())
        {
            setToolTip(tr("This node is deprecated and will be removed in a future relase."));
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

NodeEvalState
Node::nodeEvalState() const
{
    auto* model = pimpl->dataInterface;
    if (!model)
    {
        return NodeEvalState::Invalid;
    }
    return model->nodeEvalState(this->uuid());
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

Node::NodeDataPtr
Node::nodeData(PortId id) const
{
    auto* model = pimpl->dataInterface;
    if (!model)
    {
        gtWarning().nospace()
            << objectName() << ": "
            << tr("Failed to access node data, evaluation model not found!");
        return {};
    }

    return model->nodeData(this->uuid(), id);
}

bool
Node::setNodeData(PortId id, NodeDataPtr data)
{
    auto* model = pimpl->dataInterface;
    if (!model)
    {
        gtWarning().nospace()
            << objectName() << ": "
            << tr("Failed to set node data, evaluation model not found!");
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

void
Node::eval()
{
    // nothing to do here
}

void
Node::evalFailed()
{
    auto* model = pimpl->dataInterface;
    if (!model)
    {
        gtWarning().nospace()
            << objectName() << ": "
            << tr("Failed to set node evaluation status, evaluation model not found!");
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
 * members of a Node outside the node.
 */
class intelli::INode
{
    INode() = delete;

public:

    static void evaluateNode(Node& node)
    {
        node.eval();
    }

    // cppcheck-suppress constParameter
    static void setNodeDataInterface(Node& node, NodeDataInterface* interface)
    {
        node.pimpl->dataInterface = interface;
    }

    // cppcheck-suppress constParameter
    static NodeDataInterface* nodeDataInterface(Node& node)
    {
        return node.pimpl->dataInterface;
    }

    static bool triggerNodeEvaluation(Node& node, NodeDataInterface& interface)
    {
        auto evalMode = node.nodeEvalMode();
        if (evalMode == NodeEvalMode::NoEvaluationRequired) return true;

        size_t evalFlag = (size_t)node.nodeEvalMode();
        if (evalFlag & IsDetachedMask)
        {
            return exec::detachedEvaluation(node, interface);
        }
        if (evalFlag & IsBlockingMask)
        {
            return exec::blockingEvaluation(node, interface);
        }

        gtError().nospace()
            << node.objectName() << ": "
            << QObject::tr("Unhandled eval mode! (%1)").arg((size_t)evalFlag);

        return false;
    }
};

bool
intelli::exec::detachedEvaluation(Node& node, NodeDataInterface& model)
{
    auto executor = node.findChild<DetachedExecutor*>();
    if (executor && !executor->canEvaluateNode())
    {
        gtError() << QObject::tr("Node %1 (%2) already has an executor!")
                         .arg(node.id()).arg(node.uuid());
        return false;
    }

    if (!executor) executor = new DetachedExecutor;
    executor->setParent(&node);

    if (!executor->evaluateNode(node, model))
    {
        delete executor;
        return false;
    }

    return true;
}

bool
intelli::exec::blockingEvaluation(Node& node, NodeDataInterface& model)
{
    auto cmd = model.nodeEvaluation(node.uuid());
    Q_UNUSED(cmd);

    // cleanup routine
    auto finally = gt::finally([&node](){
        emit node.computingFinished();
    });
    Q_UNUSED(finally);

    emit node.computingStarted();

    INode::setNodeDataInterface(node, &model);
    INode::evaluateNode(node);

    return true;
}

bool
intelli::exec::triggerNodeEvaluation(Node& node, NodeDataInterface& model)
{
    return INode::triggerNodeEvaluation(node, model);
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
