/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/node.h"

#include "intelli/nodedatafactory.h"
#include "intelli/graph.h"
#include "intelli/nodeexecutor.h"
#include "intelli/nodedatamodel.h"
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
    pimpl(std::make_unique<NodeImpl>(modelName))
{
    setFlag(UserDeletable, true);
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

    bool hide = !gtApp || !gtApp->devMode();
    pimpl->posX.hide(hide);
    pimpl->posY.hide(hide);
    pimpl->sizeWidth.hide(hide);
    pimpl->sizeHeight.hide(hide);

    setCaption(modelName);
    
    connect(this, &Node::portInserted,
            this, &Node::nodeChanged);
    connect(this, &Node::portDeleted,
            this, &Node::nodeChanged);
    connect(this, &QObject::objectNameChanged,
            this, &Node::nodeChanged);

    connect(&pimpl->isActive, &GtAbstractProperty::changed, this, [this](){
        if (pimpl->isActive) emit triggerNodeEvaluation();
        emit isActiveChanged();
    });

    connect(this, &Node::computingStarted, this, [this](){
        setNodeFlag(NodeFlag::Evaluating, true);
    }, Qt::DirectConnection);

    connect(this, &Node::computingFinished, this, [this](){
        setNodeFlag(NodeFlag::Evaluating, false);
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
    emit isActiveChanged();
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
    enable ? pimpl->flags |= flag : pimpl->flags &= ~flag;
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
    auto find = pimpl->find(id);
    if (!find) return false;

    // notify model
    emit portAboutToBeDeleted(find.type, find.idx);
    auto finally = gt::finally([type = find.type, idx = find.idx, this](){
        emit portDeleted(type, idx);
    });

    find.ports->erase(std::next(find.ports->begin(), find.idx));

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
        auto iter = findPort(*ports, id);

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

    auto iter = findPort(ports, id);

    if (iter != ports.end())
    {
        return PortIndex::fromValue(std::distance(ports.begin(), iter));
    }

    return PortIndex{};
}

Node::PortType
Node::portType(PortId id) const noexcept(false)
{
    auto find = pimpl->find(id);
    if (!find) return PortType::NoType;

    return find.type;
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
    auto* graph = qobject_cast<Graph const*>(parent());
    if (!graph) return false;

    return !graph->findConnectedNodes(id(), portId).empty();
}

void
Node::eval()
{
    // nothing to do here
}

bool
Node::handleNodeEvaluation(NodeDataInterface& model)
{
    switch (pimpl->evalMode)
    {
    case NodeEvalMode::ExclusiveDetached:
    case NodeEvalMode::Detached:
        return exec::detachedEvaluation(*this, model);
    case NodeEvalMode::ExclusiveBlocking:
    case NodeEvalMode::Blocking:
        return exec::blockingEvaluation(*this, model);
    }

    gtError().nospace()
        << objectName() << ": "
        << tr("Unkonw eval mode! (%1)").arg((int)pimpl->evalMode);
    return false;
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
