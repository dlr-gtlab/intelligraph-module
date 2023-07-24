/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphnode.h"

#include "private/intelligraphnode_impl.h"

#include "gt_qtutilities.h"
#include "gt_exceptions.h"

#include <QRegExpValidator>

template <typename Ports>
auto findPort(Ports&& ports, gt::ig::PortId id)
{
    return std::find_if(ports.begin(), ports.end(),
                        [id](auto const& p){
        return p.id() == id;
    });
}

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
GtIntelliGraphNode::setActive(bool isActive)
{
    pimpl->active = isActive;
}

bool
GtIntelliGraphNode::isActive() const
{
    return pimpl->active;
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
    return ports_(type);
}

std::vector<GtIntelliGraphNode::PortData>&
GtIntelliGraphNode::ports_(PortType type) const noexcept(false)
{
    switch (type)
    {
    case PortType::In:
        return pimpl->inPorts;
    case PortType::Out:
        return pimpl->outPorts;
    case PortType::NoType:
        break;
    }

    throw GTlabException{
        __FUNCTION__, QStringLiteral("Invalid port type specified!")
    };
}

std::vector<GtIntelliGraphNode::NodeData>&
GtIntelliGraphNode::nodeData_(PortType type) const noexcept(false)
{
    switch (type)
    {
    case PortType::In:
        return pimpl->inData;
    case PortType::Out:
        return pimpl->outData;
    case PortType::NoType:
        break;
    }

    throw GTlabException{
        __FUNCTION__, QStringLiteral("Invalid port type specified!")
    };
}

GtIntelliGraphNode::PortId
GtIntelliGraphNode::addInPort(PortData port, PortPolicy policy) noexcept(false)
{
    return insertInPort(std::move(port), -1, policy);
}

GtIntelliGraphNode::PortId
GtIntelliGraphNode::addOutPort(PortData port, PortPolicy policy) noexcept(false)
{
    return insertOutPort(std::move(port), -1, policy);
}

GtIntelliGraphNode::PortId
GtIntelliGraphNode::insertInPort(PortData port, int idx, PortPolicy policy) noexcept(false)
{
    port.optional = policy == PortPolicy::Optional;
    return insertPort(PortType::In, std::move(port), idx);
}

GtIntelliGraphNode::PortId
GtIntelliGraphNode::insertOutPort(PortData port, int idx, PortPolicy policy) noexcept(false)
{
    port.evaluate = policy != PortPolicy::DoNotEvaluate;
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

    auto& ports = ports_(type);
    auto& data  = nodeData_(type);

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

    port.m_id = pimpl->nextPortId++;
    PortId id = ports.insert(iter, std::move(port))->m_id;
    data.resize(ports.size());
    return id;
}

bool
GtIntelliGraphNode::removePort(PortId id)
{
    PortType type = PortType::In;

    for (auto* ports : { &pimpl->inPorts, &pimpl->outPorts })
    {
        auto iter = findPort(*ports, id);

        if (iter != ports->end())
        {
            auto idx   = std::distance(ports->begin(), iter);
            auto& data = nodeData_(type);

            assert(ports->size() == data.size());

            // notify model
            PortIndex pidx = PortIndex::fromValue(idx);
            emit portAboutToBeDeleted(type, pidx);
            auto finally = gt::finally([=](){ emit portDeleted(type, pidx); });

            ports->erase(iter);
            data.erase(std::next(data.begin(), idx));
            return true;
        }
        // switch type
        type = PortType::Out;
    }

    return false;
}

GtIntelliGraphNode::NodeData const&
GtIntelliGraphNode::nodeData(PortId id) const
{
    PortIndex idx;
    PortType type = PortType::In;

    for (auto* ports : { &pimpl->inPorts, &pimpl->outPorts })
    {
        auto iter = findPort(*ports, id);

        if (iter != ports->end())
        {
            idx = PortIndex::fromValue(std::distance(ports->begin(), iter));
            break;
        }
        // switch type
        type = PortType::Out;
    }

    auto& data = nodeData_(type);
    if (idx >= data.size())
    {
        gtWarning() << tr("PortId out of bound!") << id << idx;
        static const NodeData dummy{};
        return dummy;
    }

    return data.at(idx);
}

GtIntelliGraphNode::PortData*
GtIntelliGraphNode::port(PortId id) noexcept
{
    for (auto* ports : { &pimpl->inPorts, &pimpl->outPorts })
    {
        auto iter = findPort(*ports, id);

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

    auto iter = findPort(ports, id);

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

    pimpl->state = EvalRequired;

    emit inputDataRecieved(idx);

    updateNode();

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
    if (pimpl->state == EvalRequired) updatePort(idx);

    return pimpl->outData.at(idx);
}

void
GtIntelliGraphNode::updateNode()
{
    return pimpl->executor.evaluateNode(*this);
}

void
GtIntelliGraphNode::updatePort(gt::ig::PortIndex idx)
{
    return pimpl->executor.evaluatePort(*this, idx);
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
