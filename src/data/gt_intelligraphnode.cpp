/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphnode.h"

#include "gt_objectfactory.h"
#include "gt_objectmemento.h"
#include "gt_qtutilities.h"

#include <QJsonObject>
#include <QJsonObject>

GtIntelliGraphNode::GtIntelliGraphNode(QString const& caption, GtObject* parent) :
    GtObject(parent),
    m_id("id", tr("Node Id"), tr("Node Id")),
    m_posX("posX", tr("x-Pos"), tr("x-Position")),
    m_posY("posY", tr("y-Pos"), tr("y-Position")),
    m_caption(caption)
{
    setFlag(UserDeletable, true);
    setFlag(UserRenamable, true);

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
GtIntelliGraphNode::setNodeFlag(NodeFlag flag, bool enable)
{
    enable ? m_flags |= flag : m_flags &= ~flag;
}

void
GtIntelliGraphNode::setId(gt::ig::NodeId id)
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

std::unique_ptr<GtIntelliGraphNode>
GtIntelliGraphNode::fromJson(const QJsonObject& json) noexcept(false)
{
    constexpr auto invalid = QtNodes::InvalidPortIndex;

    auto internals = json["internal-data"].toObject();
    auto classname = internals["model-name"].toString();

    auto node = GtIntelliGraphNodeFactory::instance().newNode(classname);

    node->m_id = json["id"].toInt(invalid);

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

bool
GtIntelliGraphNode::isValid() const
{
    constexpr auto invalid = QtNodes::InvalidPortIndex;

    return id() != invalid;
}

bool
GtIntelliGraphNode::isValid(const QString& modelName)
{
    return isValid() && modelName == this->modelName();
}

void
GtIntelliGraphNode::updateObjectName()
{
    gt::ig::setUniqueName(*this, m_caption);
}
