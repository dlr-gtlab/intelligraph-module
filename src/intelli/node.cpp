/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/node.h"

#include "intelli/exec/executorfactory.h"
#include "intelli/private/node_impl.h"
#include "intelli/private/utils.h"

#include "gt_qtutilities.h"

#include <QRegExpValidator>

using namespace intelli;

Node::Node(QString const& modelName, GtObject* parent) :
    GtObject(parent),
    pimpl(std::make_unique<NodeImpl>(modelName))
{
    setFlag(UserDeletable, true);
    setFlag(UserRenamable, false);

    static const QString cat = QStringLiteral("Node");
    registerProperty(pimpl->id, cat);
    registerProperty(pimpl->posX, cat);
    registerProperty(pimpl->posY, cat);
    registerProperty(pimpl->sizeWidth, cat);
    registerProperty(pimpl->sizeHeight, cat);

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
}

Node::~Node() = default;

void
Node::setExecutor(ExecutionMode executorMode)
{
    auto executor = ExecutorFactory::makeExecutor(executorMode);

    if (pimpl->executor)
    {
        if (!pimpl->executor->isReady())
        {
            gtWarning() << tr("Replacing executor of node '%1', which is not ready!")
                               .arg(objectName());
        }
    }
    emit computingFinished();
    pimpl->executor = std::move(executor);
}

void
Node::setId(NodeId id)
{
    pimpl->id = id;
}

NodeId
Node::id() const
{
    return NodeId{fromInt(pimpl->id)};
}

void
Node::setPos(Position pos)
{
    if (this->pos() != pos)
    {
        pimpl->posX = pos.x();
        pimpl->posY = pos.y();
        changed();
    }
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

void
Node::setCaption(QString const& caption)
{
    gt::setUniqueName(*this, caption);
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
    auto& data  = pimpl->nodeData(type);

    assert(ports.size() == data.size());

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
    data.resize(ports.size());

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
    find.data->erase(std::next(find.data->begin(), find.idx));
    return true;
}

Node::NodeDataPtr const&
Node::nodeData(PortId id) const
{
    auto find = pimpl->find(id);
    if (!find)
    {
        gtWarning() << tr("PortId '%1' not found!").arg(id);
        static const NodeDataPtr dummy{};
        return dummy;
    }

    assert (find.idx < find.data->size());

    return find.data->at(find.idx);
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

Node::NodeDataPtr const&
Node::inData(PortIndex idx)
{
    if (idx >= pimpl->inData.size())
    {
        static NodeDataPtr const dummy;
        return dummy;
    }

    gtTrace().verbose().nospace()
        << "### Getting in data:  '" << objectName()
        << "' at input idx  '" << idx << "': " << pimpl->inData.at(idx);

    return pimpl->inData.at(idx);
}

bool
Node::setInData(PortIndex idx, NodeDataPtr data)
{
    if (idx >= pimpl->inData.size())
    {
        gtWarning() << tr("Setting in data failed! Port index %1 out of bounds!").arg(idx)
                    << gt::brackets(caption());
        return false;
    }

    gtTrace().verbose().nospace()
        << "### Setting in data:  '" << objectName()
        << "' at input idx  '" << idx << "': " << data;

    pimpl->inData.at(idx) = std::move(data);

    pimpl->requiresEvaluation = true;

    emit inputDataRecieved(idx);

    updateNode();

    return true;
}

Node::NodeDataPtr const&
Node::outData(PortIndex idx)
{
    if (idx >= pimpl->outData.size())
    {
        static NodeDataPtr const dummy;
        return dummy;
    }

    gtTrace().verbose().nospace()
        << "### Getting out data: '" << objectName()
        << "' at output idx '" << idx << "': " << pimpl->outData.at(idx);

    // trigger node update if no input data is available
    if (pimpl->requiresEvaluation) updatePort(idx);

    return pimpl->outData.at(idx);
}

bool
Node::setOutData(PortIndex idx, NodeDataPtr data)
{
    if (idx >= pimpl->outData.size())
    {
        gtWarning() << tr("Setting out data failed! Port index %1 out of bounds!").arg(idx)
                    << gt::brackets(caption());
        return false;
    }

    gtTrace().verbose().nospace()
        << "### Setting out data:  '" << objectName()
        << "' at output idx  '" << idx << "': " << data;

    auto& out = pimpl->outData.at(idx);

    out = std::move(data);

    out ? emit outDataUpdated(idx) : emit outDataInvalidated(idx);

    return true;
}

void
Node::updateNode()
{
    if (pimpl->executor && !(nodeFlags() & DoNotEvaluate))
    {
        pimpl->requiresEvaluation = false;
        // if we failed to start the evaluation the node needs to be evaluated
        // at a later point
        bool successfullyStarted = pimpl->executor->evaluateNode(*this);
        pimpl->requiresEvaluation = !successfullyStarted;
    }
}

void
Node::updatePort(PortIndex idx)
{
    if (pimpl->executor && !(nodeFlags() & DoNotEvaluate))
    {
        pimpl->requiresEvaluation = false;
        // if we failed to start the evaluation the node needs to be evaluated
        // at a later point
        bool successfullyStarted = pimpl->executor->evaluatePort(*this, idx);
        pimpl->requiresEvaluation = !successfullyStarted;
    }
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
