/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphnode.h"

#include "gt_intelligraphnodefactory.h"
#include "gt_igvolatileptr.h"

#include "gt_intproperty.h"
#include "gt_doubleproperty.h"
#include "gt_stringproperty.h"
#include "gt_regexp.h"
#include "gt_objectfactory.h"
#include "gt_objectmemento.h"
#include "gt_qtutilities.h"
#include "gt_exceptions.h"

#include <QJsonObject>
#include <QRegExpValidator>

gt::log::Stream& operator<<(gt::log::Stream& s, GtIntelliGraphNode::NodeData const& data)
{
    return s << (data ? data->metaObject()->className() : "nullptr");
}

struct GtIntelliGraphNode::Impl
{
    Impl(QString const& name) : modelName(name) {}

    /// node id
    GtIntProperty id{"id", tr("Node Id"), tr("Node Id")};
    /// x position of node
    GtDoubleProperty posX{"posX", tr("x-Pos"), tr("x-Position")};
    /// y position of node
    GtDoubleProperty posY{"posY", tr("y-Pos"), tr("y-Position")};
    /// caption string
    QString modelName;
    /// model name string
    GtStringProperty caption{
        "caption", tr("Caption"), tr("Node Capton"), modelName,
        new QRegExpValidator{gt::re::woUmlauts()}
    };
    /// ports
    std::vector<PortData> inPorts, outPorts{};
    /// data
    std::vector<NodeData> inData, outData{};
    /// owning pointer to widget, may be deleted earlier
    gt::ig::volatile_ptr<QWidget> widget{};
    /// factory for creating the widget
    WidgetFactory widgetFactory{};
    /// node flags
    NodeFlags flags{gt::ig::NoFlag};
    /// iterator for the next port id
    PortId nextPortId{0};

    State state{EvalRequired};

    bool active{false};

    bool canEvaluate() const
    {
        assert(inData.size() == inPorts.size());

        PortIndex idx{0};
        for (auto const& data : inData)
        {
            auto const& p = inPorts.at(idx++);

            // check if data is requiered and valid
            if (!p.optional && !data) return false;
        }

        return true;
    }
};

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
    registerProperty(pimpl->caption, cat);

    pimpl->id.setReadOnly(true);
    pimpl->posX.setReadOnly(true);
    pimpl->posY.setReadOnly(true);

    updateObjectName();

    connect(this, &GtIntelliGraphNode::portInserted,
            this, &GtIntelliGraphNode::nodeChanged);
    connect(this, &GtIntelliGraphNode::portDeleted,
            this, &GtIntelliGraphNode::nodeChanged);
    connect(&pimpl->caption, &GtAbstractProperty::changed,
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
    gt::setUniqueName(*this, pimpl->caption);
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
GtIntelliGraphNode::setCaption(QString caption)
{
    pimpl->caption = std::move(caption);
    emit pimpl->caption.changed();
    updateObjectName();
}

QString const&
GtIntelliGraphNode::caption() const
{
    return pimpl->caption.get();
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

    gtDebug().verbose().nospace()
        << "### Setting in data:  '" << objectName()
        << "' at idx '" << idx << "': " << data;

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

    gtDebug().verbose().nospace()
        << "### Getting out data: '" << objectName()
        << "' at idx '" << idx << "': " << pimpl->outData.at(idx);

    // trigger node update if no input data is available
    if (pimpl->state == EvalRequired) updatePort(idx);

    return pimpl->outData.at(idx);
}

void
GtIntelliGraphNode::updateNode()
{
    updatePort(gt::ig::invalid<PortIndex>());
}

void
GtIntelliGraphNode::updatePort(gt::ig::PortIndex idx)
{
    if (pimpl->state == Evaluating)
    {
        gtWarning().verbose()
            << tr("Node already evaluating!") << gt::brackets(objectName());
        return;
    }

    if (!isActive())
    {
        gtWarning().verbose()
            << tr("Node is not active!") << gt::brackets(objectName());
        return;
    }

    bool canEvaluate = pimpl->canEvaluate();

    if (!canEvaluate)
    {
        // not aborting here to allow the triggering of the invalidated signals
        pimpl->state = EvalRequired;
        gtWarning().verbose()
            << tr("Node not ready for evaluation!") << gt::brackets(objectName());
    }

    // eval helper
    const auto evalPort = [=](PortId id, NodeData* out = nullptr){
        pimpl->state = Evaluating;

        gtDebug().verbose().nospace()
            << "### Evaluating node:  '" << objectName()
            << "' at output id '" << id << "'";

        auto tmp = eval(id);

        if (out) *out = std::move(tmp);

        pimpl->state = Evaluated;

        emit evaluated(idx);
    };

    // update helper
    const auto updateOutData = [=](PortIndex idx, PortData const& port){
        if (!port.evaluate) return;
        // invalidate out data
        if (!canEvaluate) return emit outDataInvalidated(idx);

        auto& out = pimpl->outData.at(idx);

        evalPort(port.m_id, &out);

        out ? emit outDataUpdated(idx) : emit outDataInvalidated(idx);
    };

    // update single port
    if (idx != gt::ig::invalid<PortIndex>())
    {
        if (idx >= pimpl->outPorts.size()) return;

        return updateOutData(idx, pimpl->outPorts.at(idx));
    }

    // trigger eval if no outport exists
    if (pimpl->outPorts.empty() && !pimpl->inPorts.empty())
    {
        evalPort(gt::ig::invalid<PortId>());
        return;
    }

    // update all ports
    idx = PortIndex{0};
    for (auto const& port : pimpl->outPorts)
    {
        updateOutData(idx++, port);
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
        pimpl->widget = gt::ig::volatile_ptr<QWidget>(tmp.release());
    }
}

std::unique_ptr<GtIntelliGraphNode>
GtIntelliGraphNode::fromJson(const QJsonObject& json) noexcept(false)
{
    auto internals = json["internal-data"].toObject();
    auto classname = internals["model-name"].toString();

    auto node = GtIntelliGraphNodeFactory::instance().newNode(classname);

    node->pimpl->id = json["id"].toInt(gt::ig::invalid<PortId>());

    auto position = json["position"];
    node->pimpl->posX  = position["x"].toDouble();
    node->pimpl->posY  = position["y"].toDouble();

    node->mergeNodeData(internals);

    return node;
}

bool
GtIntelliGraphNode::mergeNodeData(const QJsonObject& internals)
{
    auto mementoData = internals["memento"].toString();

    GtObjectMemento memento(mementoData.toUtf8());

    if (memento.isNull() || !memento.mergeTo(*this, *gtObjectFactory))
    {
        gtWarning()
            << tr("Failed to restore memento for '%1', object may be incomplete")
               .arg(objectName());
        gtWarning().medium()
            << tr("Memento:") << mementoData;
        return false;
    }
    return true;
}

QJsonObject
GtIntelliGraphNode::toJson(bool clone) const
{
    QJsonObject json;
    json["id"] = pimpl->id.get();

    QJsonObject position;
    position["x"] = pimpl->posX.get();
    position["y"] = pimpl->posY.get();
    json["position"] = position;

    QJsonObject internals;
    internals["model-name"] = modelName();
    internals["memento"] = static_cast<QString>(toMemento(clone).toByteArray());
    json["internal-data"] = internals;

    return json;
}
