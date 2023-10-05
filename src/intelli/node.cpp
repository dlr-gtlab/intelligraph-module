/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/node.h"

#include "intelli/graphexecmodel.h"
#include "intelli/nodeexecutor.h"
#include "intelli/private/node_impl.h"

#include <gt_qtutilities.h>

#include <QRegExpValidator>
#include <QVBoxLayout>

using namespace intelli;

std::unique_ptr<QWidget>
intelli::makeWidget()
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

    setCaption(modelName);
    
    connect(this, &Node::portInserted,
            this, &Node::nodeChanged);
    connect(this, &Node::portDeleted,
            this, &Node::nodeChanged);
    connect(this, &QObject::objectNameChanged,
            this, &Node::nodeChanged);

    connect(&pimpl->isActive, &GtAbstractProperty::changed,
            this, &Node::nodeStateChanged);

    connect(this, &Node::triggerNodeEvaluation, this, [this](){
        emit triggerPortEvaluation(invalid<PortId>());
    });

    connect(this, &Node::computingStarted, this, [this](){
        setNodeFlag(NodeFlag::Evaluating, true);
        // reset requires evaluation
        setNodeFlag(NodeFlag::RequiresEvaluation, false);
        emit nodeStateChanged();
    }, Qt::DirectConnection);
    connect(this, &Node::computingFinished, this, [this](){
        setNodeFlag(NodeFlag::Evaluating, false);
        emit nodeStateChanged();
    }, Qt::DirectConnection);
}

Node::~Node()
{
    emit nodeAboutToBeDeleted(id());
}

void
Node::setActive(bool active)
{
    pimpl->isActive = active;
    emit nodeStateChanged();
}

bool
Node::isActive() const
{
    return pimpl->isActive;
}

void
Node::invalidate()
{
    setNodeFlag(RequiresEvaluation);
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

NodeFlags
Node::nodeFlags() const
{
    return pimpl->flags;
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

std::vector<Node::PortData> const&
Node::ports(PortType type) const noexcept(false)
{
    return pimpl->ports(type);
}

PortId
Node::addInPort(PortData port, PortPolicy policy) noexcept(false)
{
    return insertInPort(std::move(port), -1, policy);
}

PortId
Node::addOutPort(PortData port) noexcept(false)
{
    return insertOutPort(std::move(port), -1);
}

PortId
Node::insertInPort(PortData port, int idx, PortPolicy policy) noexcept(false)
{
    port.optional = policy == PortPolicy::Optional;
    return insertPort(PortType::In, std::move(port), idx);
}

PortId
Node::insertOutPort(PortData port, int idx) noexcept(false)
{
    return insertPort(PortType::Out, std::move(port), idx);
}

PortId
Node::insertPort(PortType type, PortData port, int idx) noexcept(false)
{
    if (port.typeId.isEmpty())
    {
        gtWarning() << tr("Invalid port typeId specified!");
        return PortId{};
    }

    auto& ports = pimpl->ports(type);

    auto iter = ports.end();

    if (idx >= 0 && static_cast<size_t>(idx) < ports.size())
    {
        iter = std::next(ports.begin(), idx);
    }

    // notify model
    PortIndex pidx = PortIndex::fromValue(std::distance(ports.begin(), iter));
    emit portAboutToBeInserted(type, pidx);
    auto finally = gt::finally([=](){ emit portInserted(type, pidx); });

    PortId id = pimpl->nextPortId++;
    port.m_id = id;
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
    auto* model = nodeDataInterface();
    if (!model)
    {
        gtWarning() << tr("Evaluation model not found!")
                    << gt::brackets(objectName());
        return {};
    }

    return model->nodeData(this->id(), id);
}

Node::PortData*
Node::port(PortId id) noexcept
{
    for (auto* ports : { &pimpl->inPorts, &pimpl->outPorts })
    {
        auto iter = findPort(*ports, id);

        if (iter != ports->end()) return &(*iter);
    }

    return nullptr;
}

Node::PortData const*
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

Node::NodeDataPtr
Node::eval(PortId)
{
    // nothing to do here
    return {};
}

NodeDataInterface*
Node::nodeDataInterface()
{
    if (pimpl->dataInterface) return pimpl->dataInterface;

    return NodeExecutor::accessExecModel(*this);
}

NodeDataInterface const*
Node::nodeDataInterface() const
{
    return const_cast<Node*>(this)->nodeDataInterface();
}

bool
Node::handleNodeEvaluation(GraphExecutionModel& model, PortId portId)
{
    return detachedEvaluation(*this, model, portId);
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

QWidget*
Node::embeddedWidget()
{
    if (!pimpl->widget) initWidget();

    return pimpl->widget;
}

void
Node::initWidget()
{
    if (pimpl->widgetFactory)
    {
        auto tmp = pimpl->widgetFactory(*this);
        auto* widget = tmp.get();
        pimpl->widget = volatile_ptr<QWidget>(tmp.release());

        if (!widget || !(nodeFlags() & Resizable)) return;

        auto size = this->size();
        if (size.isValid()) widget->resize(size);
    }
}
