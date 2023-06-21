/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphnode.h"

#include "gt_intelligraphnodefactory.h"
#include "gt_objectfactory.h"
#include "gt_objectmemento.h"
#include "gt_qtutilities.h"
#include "gt_exceptions.h"

#include <QJsonObject>

GtIntelliGraphNode::GtIntelliGraphNode(QString const& modelName, GtObject* parent) :
    GtObject(parent),
    m_id("id", tr("Node Id"), tr("Node Id")),
    m_posX("posX", tr("x-Pos"), tr("x-Position")),
    m_posY("posY", tr("y-Pos"), tr("y-Position")),
    m_modelName(modelName),
    m_caption(modelName)
{
    setFlag(UserDeletable, true);
    setFlag(UserRenamable, false);

    static const QString cat = QStringLiteral("Node");
    registerProperty(m_id, cat);
    registerProperty(m_posX, cat);
    registerProperty(m_posY, cat);

    m_id.setReadOnly(true);
    m_posX.setReadOnly(true);
    m_posY.setReadOnly(true);

    updateObjectName();
}

void
GtIntelliGraphNode::setId(NodeId id)
{
    m_id = id;
}

void
GtIntelliGraphNode::setPos(QPointF pos)
{
    if (this->pos() != pos)
    {
        m_posX = pos.x();
        m_posY = pos.y();
        changed();
    }
}

bool
GtIntelliGraphNode::isValid() const
{
    return id() != gt::ig::invalid<PortId>();
}

bool
GtIntelliGraphNode::isValid(const QString& modelName)
{
    return isValid() && modelName == this->modelName();
}

void
GtIntelliGraphNode::updateObjectName()
{
    gt::setUniqueName(*this, m_caption);
}

void
GtIntelliGraphNode::setNodeFlag(NodeFlag flag, bool enable)
{
    enable ? m_flags |= flag : m_flags &= ~flag;
}

void
GtIntelliGraphNode::setCaption(QString caption)
{
    m_caption = std::move(caption);
}

QString const&
GtIntelliGraphNode::caption() const
{
    return m_caption;
}

void
GtIntelliGraphNode::setModelName(QString name)
{
    m_modelName = std::move(name);
}

QString const&
GtIntelliGraphNode::modelName() const
{
    return m_modelName;
}

std::vector<GtIntelliGraphNode::PortData> const&
GtIntelliGraphNode::ports(PortType type) const noexcept(false)
{
    switch (type)
    {
    case PortType::In:
        return m_inPorts;
    case PortType::Out:
        return m_outPorts;
    case PortType::None:
        break;
    }

    throw GTlabException{
        __FUNCTION__, QStringLiteral("Invalid port type specified!")
    };
}

std::vector<GtIntelliGraphNode::PortData>&
GtIntelliGraphNode::ports_(PortType type) noexcept(false)
{
    return const_cast<std::vector<GtIntelliGraphNode::PortData>&>(
        const_cast<GtIntelliGraphNode const*>(this)->ports(type)
    );
}

std::vector<GtIntelliGraphNode::NodeData>&
GtIntelliGraphNode::portData_(PortType type) noexcept(false)
{
    switch (type)
    {
    case PortType::In:
        return m_inData;
    case PortType::Out:
        return m_outData;
    case PortType::None:
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
GtIntelliGraphNode::addOutPort(PortData port) noexcept(false)
{
    return insertOutPort(std::move(port), -1);
}

GtIntelliGraphNode::PortId
GtIntelliGraphNode::insertInPort(PortData port, int idx, PortPolicy policy) noexcept(false)
{
    port.optional = policy == PortPolicy::Optional;
    return insertPort(PortType::In, std::move(port), -1);
}

GtIntelliGraphNode::PortId
GtIntelliGraphNode::insertOutPort(PortData port, int idx) noexcept(false)
{
    return insertPort(PortType::Out, std::move(port), -1);
}

GtIntelliGraphNode::PortId
GtIntelliGraphNode::insertPort(PortType type, PortData port, int idx) noexcept(false)
{
    if (port.typeId.isEmpty())
    {
        gtWarning() << tr("Invalid port typeId specified!");
        return gt::ig::invalid<PortId>();
    }

    auto& ports = ports_(type);
    auto& data  = portData_(type);

    auto iter = ports.end();

    if (idx >= 0 && static_cast<size_t>(idx) < ports.size())
    {
        iter = std::next(ports.begin(), idx);
    }

    port.m_id = m_nextPortId++;
    PortId id = ports.insert(iter, std::move(port))->m_id;
    data.resize(ports.size());
    return id;
}

bool
GtIntelliGraphNode::removePort(PortId id)
{
    for (auto* ports : { &m_inPorts, &m_outPorts })
    {
        auto iter = std::find_if(ports->begin(), ports->end(),
                                 [id](PortData const& p){
            return p.m_id == id;
        });

        if (iter != ports->end())
        {
            ports->erase(iter);
            return true;
        }
    }

    return false;
}

GtIgNodeData const*
GtIntelliGraphNode::portData(PortId inId) const
{
    PortIndex idx = portIndex(PortType::In, inId);

    if (idx == gt::ig::invalid<PortIndex>())
    {
        idx = portIndex(PortType::Out, inId);
    }

    if (idx >= m_inData.size())
    {
        gtWarning() << tr("PortId out of bound!") << inId << idx;
        return {};
    }

    return m_inData.at(idx).get();
}

GtIntelliGraphNode::PortData const*
GtIntelliGraphNode::port(PortId id) const noexcept
{
    for (auto* ports : { &m_inPorts, &m_outPorts })
    {
        auto iter = std::find_if(ports->begin(), ports->end(),
                                 [id](PortData const& p){
            return p.m_id == id;
        });

        if (iter != ports->end()) return &(*iter);
    }

    return nullptr;
}

GtIntelliGraphNode::PortData const*
GtIntelliGraphNode::port(PortType type, PortIndex idx) const noexcept
{
    auto& ports = this->ports(type);

    if (idx >= ports.size()) return nullptr;

    return &ports.at(idx);
}

GtIntelliGraphNode::PortIndex
GtIntelliGraphNode::portIndex(PortType type, PortId id) const noexcept(false)
{
    auto& ports = this->ports(type);

    auto iter = std::find_if(ports.begin(), ports.end(),
                             [id](PortData const& p){
         return p.m_id == id;
     });

    if (iter != ports.end()) return std::distance(ports.begin(), iter);

    return gt::ig::invalid<PortId>();
}

GtIntelliGraphNode::PortId
GtIntelliGraphNode::portId(PortType type, PortIndex idx) const noexcept(false)
{
    auto const* p = port(type, idx);
    return p ? p->m_id : gt::ig::invalid<PortId>();
}

GtIntelliGraphNode::NodeData
GtIntelliGraphNode::eval(PortId)
{
    // nothing to do here
    return {};
}

void
GtIntelliGraphNode::setInData(PortIndex idx, NodeData data)
{
    if (idx >= m_inData.size()) return;

    gtDebug() << "SET IN DATA: " << idx << metaObject()->className() << (data ? data->metaObject()->className() : "<nullptr>");

    m_inData.at(idx) = std::move(data);

    m_state = EvalRequired;

    emit inputDataRecieved(idx);

    updateNode();
}

GtIntelliGraphNode::NodeData
GtIntelliGraphNode::outData(PortIndex idx)
{
    if (idx >= m_outData.size()) return {};

    gtDebug() << "OUT DATA:    " << idx << metaObject()->className();

    // trigger node update if no input data is available
    if (m_state == EvalRequired) updatePort(idx);

    return m_outData.at(idx);
}

void
GtIntelliGraphNode::updateNode()
{
    updatePort(gt::ig::invalid<PortIndex>());
}

void
GtIntelliGraphNode::updatePort(gt::ig::PortIndex idx)
{
    if (m_state == Evaluating)
    {
        gtWarning() << tr("Node already evaluating!") << metaObject()->className();
        return;
    }

    bool canEval = canEvaluate();

    // update helper
    const auto updateOutData = [=](PortIndex idx, PortData const& port){
        auto& out = m_outData.at(idx);

        if (!canEval)
        {
            m_state = EvalRequired;
            emit outDataInvalidated(idx);
            gtWarning() << tr("Node cannot evaluate!") << metaObject()->className();
            return;
        }

        m_state = Evaluating;

        gtDebug() << "EVALUATING:  BEFORE " << metaObject()->className() << out;

        out = eval(port.m_id);

        gtDebug() << "EVALUATING:  AFTER " << metaObject()->className() << out;

        m_state = Evaluated;

        emit (out ?  outDataUpdated(idx) : outDataInvalidated(idx));

    };

    // update single port
    if (idx != gt::ig::invalid<PortIndex>())
    {
        if (idx >= m_outPorts.size()) return;

        return updateOutData(idx, m_outPorts.at(idx));
    }

    // update all ports
    idx = 0;
    for (auto const& port : m_outPorts)
    {
        updateOutData(idx++, port);
    }
}

void
GtIntelliGraphNode::registerWidgetFactory(WidgetFactory factory)
{
    m_widgetFactory = std::move(factory);
}

QWidget*
GtIntelliGraphNode::embeddedWidget()
{
    if (!m_widget) initWidget();

    return m_widget;
}

void
GtIntelliGraphNode::initWidget()
{
    if (!m_widgetFactory) return;

    m_widget = gt::ig::volatile_ptr<QWidget>(m_widgetFactory(*this).release());
}

bool
GtIntelliGraphNode::canEvaluate() const
{
    assert(m_inData.size() == m_inPorts.size());

    PortIndex idx = 0;
    for (auto const& data : m_inData)
    {
        auto const& p = m_inPorts.at(idx++);
        if (!p.optional && !data) return false;
    }

    return true;
}

std::unique_ptr<GtIntelliGraphNode>
GtIntelliGraphNode::fromJson(const QJsonObject& json) noexcept(false)
{
    auto internals = json["internal-data"].toObject();
    auto classname = internals["model-name"].toString();

    auto node = GtIntelliGraphNodeFactory::instance().newNode(classname);

    node->m_id = json["id"].toInt(gt::ig::invalid<PortId>());

    auto position = json["position"];
    node->m_posX  = position["x"].toDouble();
    node->m_posY  = position["y"].toDouble();

    node->mergeJsonMemento(internals);

    return node;
}

void
GtIntelliGraphNode::mergeJsonMemento(const QJsonObject& internals)
{
    auto mementoData = internals["memento"].toString();

    GtObjectMemento memento(mementoData.toUtf8());

    if (memento.isNull() || !memento.mergeTo(*this, *gtObjectFactory))
    {
        gtWarning() << tr("Failed to restore memento for '%1'!")
                       .arg(objectName())
                    << tr("Object may be incomplete");
    }
}

QJsonObject
GtIntelliGraphNode::toJson() const
{
    QJsonObject json;
    json["id"] = m_id.get();

    QJsonObject position;
    position["x"] = m_posX.get();
    position["y"] = m_posY.get();
    json["position"] = position;

    QJsonObject internals;
    internals["model-name"] = modelName();
    toJsonMemento(internals);
    json["internal-data"] = internals;

    return json;
}

void
GtIntelliGraphNode::toJsonMemento(QJsonObject& internals) const
{
    internals["memento"] = static_cast<QString>(toMemento().toByteArray());
}
