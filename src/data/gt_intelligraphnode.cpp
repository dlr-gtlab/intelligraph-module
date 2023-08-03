/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphnode.h"

#include "gt_intelligraphexecutorfactory.h"
#include "private/intelligraphnode_impl.h"
#include "private/utils.h"

#include "gt_qtutilities.h"

#include <QRegExpValidator>

GtIntelliGraphNode::GtIntelliGraphNode(QString const& modelName, GtObject* parent) :
    GtObject(parent),
    pimpl(std::make_unique<Impl>(modelName))
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

    connect(this, &GtIntelliGraphNode::portInserted,
            this, &GtIntelliGraphNode::nodeChanged);
    connect(this, &GtIntelliGraphNode::portDeleted,
            this, &GtIntelliGraphNode::nodeChanged);
    connect(this, &QObject::objectNameChanged,
            this, &GtIntelliGraphNode::nodeChanged);
    connect(this, &GtIntelliGraphNode::portChanged,
            this, &GtIntelliGraphNode::nodeChanged);
}

GtIntelliGraphNode::~GtIntelliGraphNode() = default;

void
GtIntelliGraphNode::setExecutor(ExecutorType executorType)
{
    auto executor = GtIntelliGraphExecutorFactory::makeExecutor(executorType);

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
GtIntelliGraphNode::setId(NodeId id)
{
    pimpl->id = id;
}

GtIntelliGraphNode::NodeId GtIntelliGraphNode::id() const
{
    return NodeId{gt::ig::fromInt(pimpl->id)};
}

void
GtIntelliGraphNode::setPos(QPointF pos)
{
    if (this->pos() != pos)
    {
        pimpl->posX = pos.x();
        pimpl->posY = pos.y();
        changed();
    }
}

GtIntelliGraphNode::Position GtIntelliGraphNode::pos() const
{
    return { pimpl->posX, pimpl->posY };
}

void
GtIntelliGraphNode::setSize(QSize size)
{
    if (this->size() != size)
    {
        pimpl->sizeWidth = size.width();
        pimpl->sizeHeight = size.height();
        changed();
    }
}

QSize
GtIntelliGraphNode::size() const
{
    return { pimpl->sizeWidth, pimpl->sizeHeight };
}

bool
GtIntelliGraphNode::isValid() const
{
    return id() != gt::ig::invalid<NodeId>();
}

bool
GtIntelliGraphNode::isValid(const QString& modelName)
{
    return isValid() && modelName == this->modelName();
}

void
GtIntelliGraphNode::updateObjectName()
{
    gt::setUniqueName(*this, baseObjectName());
}

void
GtIntelliGraphNode::setNodeFlag(NodeFlag flag, bool enable)
{
    enable ? pimpl->flags |= flag : pimpl->flags &= ~flag;
}

GtIntelliGraphNode::NodeFlags GtIntelliGraphNode::nodeFlags() const
{
    return pimpl->flags;
}

void
GtIntelliGraphNode::setCaption(QString const& caption)
{
    gt::setUniqueName(*this, caption);
}

QString
GtIntelliGraphNode::caption() const
{
    return objectName();
}

QString
GtIntelliGraphNode::baseObjectName() const
{
    static QRegularExpression const regExp(QStringLiteral(R"((.+)(\s?\[\d+\]))"));
    constexpr int groupNo = 1;

    auto const& name = objectName();
    auto const& match = regExp.match(name);
    return (match.capturedLength(groupNo) > 0) ? match.captured(groupNo) : name;
}

QString const&
GtIntelliGraphNode::modelName() const
{
    return pimpl->modelName;
}

std::vector<GtIntelliGraphNode::PortData> const&
GtIntelliGraphNode::ports(PortType type) const noexcept(false)
{
    return pimpl->ports(type);
}

GtIntelliGraphNode::PortId
GtIntelliGraphNode::addInPort(PortData port, PortPolicy policy) noexcept(false)
{
    return insertInPort(std::move(port), -1, policy);
}

GtIntelliGraphNode::PortId
GtIntelliGraphNode::addOutPort(PortData port) noexcept(false)
{
    return insertOutPort(std::move(port), -1);
}

GtIntelliGraphNode::PortId
GtIntelliGraphNode::insertInPort(PortData port, int idx, PortPolicy policy) noexcept(false)
{
    port.optional = policy == PortPolicy::Optional;
    return insertPort(PortType::In, std::move(port), idx);
}

GtIntelliGraphNode::PortId
GtIntelliGraphNode::insertOutPort(PortData port, int idx) noexcept(false)
{
    return insertPort(PortType::Out, std::move(port), idx);
}

GtIntelliGraphNode::PortId
GtIntelliGraphNode::insertPort(PortType type, PortData port, int idx) noexcept(false)
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
GtIntelliGraphNode::removePort(PortId id)
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

GtIntelliGraphNode::NodeData const&
GtIntelliGraphNode::nodeData(PortId id) const
{
    auto find = pimpl->find(id);
    if (!find)
    {
        gtWarning() << tr("PortId '%1' not found!").arg(id);
        static const NodeData dummy{};
        return dummy;
    }

    assert (find.idx < find.data->size());

    return find.data->at(find.idx);
}

GtIntelliGraphNode::PortData*
GtIntelliGraphNode::port(PortId id) noexcept
{
    for (auto* ports : { &pimpl->inPorts, &pimpl->outPorts })
    {
        auto iter = gt::ig::findPort(*ports, id);

        if (iter != ports->end()) return &(*iter);
    }

    return nullptr;
}

GtIntelliGraphNode::PortData const*
GtIntelliGraphNode::port(PortId id) const noexcept
{
    return const_cast<GtIntelliGraphNode*>(this)->port(id);
}

GtIntelliGraphNode::PortIndex
GtIntelliGraphNode::portIndex(PortType type, PortId id) const noexcept(false)
{
    auto& ports = this->ports(type);

    auto iter = gt::ig::findPort(ports, id);

    if (iter != ports.end())
    {
        return PortIndex::fromValue(std::distance(ports.begin(), iter));
    }

    return PortIndex{};
}

GtIntelliGraphNode::PortId
GtIntelliGraphNode::portId(PortType type, PortIndex idx) const noexcept(false)
{
    auto& ports = this->ports(type);

    if (idx >= ports.size()) return PortId{};

    return ports.at(idx).m_id;
}

GtIntelliGraphNode::NodeData
GtIntelliGraphNode::eval(PortId)
{
    // nothing to do here
    return {};
}

bool
GtIntelliGraphNode::setInData(PortIndex idx, NodeData data)
{
    if (idx >= pimpl->inData.size()) return false;

    gtTrace().verbose().nospace()
        << "### Setting in data:  '" << objectName()
        << "' at input idx  '" << idx << "': " << data;

    pimpl->inData.at(idx) = std::move(data);

    pimpl->requiresEvaluation = true;

    emit inputDataRecieved(idx);

    updateNode();

    return true;
}

bool
GtIntelliGraphNode::setOutData(PortIndex idx, NodeData data)
{
    if (idx >= pimpl->outData.size()) return false;

    gtTrace().verbose().nospace()
        << "### Setting out data:  '" << objectName()
        << "' at output idx  '" << idx << "': " << data;

    auto& out = pimpl->outData.at(idx);

    out = std::move(data);

    out ? emit outDataUpdated(idx) : emit outDataInvalidated(idx);

    return true;
}


GtIntelliGraphNode::NodeData
GtIntelliGraphNode::outData(PortIndex idx)
{
    if (idx >= pimpl->outData.size()) return {};

    gtTrace().verbose().nospace()
        << "### Getting out data: '" << objectName()
        << "' at output idx '" << idx << "': " << pimpl->outData.at(idx);

    // trigger node update if no input data is available
    if (pimpl->requiresEvaluation) updatePort(idx);

    return pimpl->outData.at(idx);
}

void
GtIntelliGraphNode::updateNode()
{
    if (pimpl->executor)
    {
        pimpl->requiresEvaluation = false;
        // if we failed to start the evaluation the node needs to be evaluated
        // at a later point
        bool successfullyStarted = pimpl->executor->evaluateNode(*this);
        pimpl->requiresEvaluation = !successfullyStarted;
    }
}

void
GtIntelliGraphNode::updatePort(gt::ig::PortIndex idx)
{
    if (pimpl->executor)
    {
        pimpl->requiresEvaluation = false;
        // if we failed to start the evaluation the node needs to be evaluated
        // at a later point
        bool successfullyStarted = pimpl->executor->evaluatePort(*this, idx);
        pimpl->requiresEvaluation = !successfullyStarted;
    }
}

void
GtIntelliGraphNode::registerWidgetFactory(WidgetFactory factory)
{
    pimpl->widgetFactory = std::move(factory);
}

QWidget*
GtIntelliGraphNode::embeddedWidget()
{
    if (!pimpl->widget) initWidget();

    return pimpl->widget;
}

void
GtIntelliGraphNode::initWidget()
{
    if (pimpl->widgetFactory)
    {
        auto tmp = pimpl->widgetFactory(*this);
        auto* widget = tmp.get();
        pimpl->widget = gt::ig::volatile_ptr<QWidget>(tmp.release());

        if (!widget || !(nodeFlags() & gt::ig::Resizable)) return;

        auto size = this->size();
        if (size.isValid()) widget->resize(size);
    }
}
