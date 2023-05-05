/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLIGRAPHNODE_H
#define GT_INTELLIGRAPHNODE_H

#include "gt_igglobals.h"
#include "gt_intelligraphnodefactory.h"

#include "gt_object.h"
#include "gt_intproperty.h"
#include "gt_doubleproperty.h"
#include "gt_stringproperty.h"
#include "gt_enumproperty.h"

namespace QtNodes { class DataFlowGraphModel; }

namespace gt
{
namespace ig
{

enum NodeFlag
{
    None = 0x0,
    Resizable = 0x1,
    CaptionInvisble = 0x4,
};

} // namespace ig

} // namespace gt

class GtIntelliGraphNode : public GtObject
{
    Q_OBJECT

    friend class GtIntelliGraphObjectModel;

public:

    using NodeId     = gt::ig::NodeId;
    using NodeFlag   = gt::ig::NodeFlag;
    using NodeFlags  = int;
    using PortType   = QtNodes::PortType;
    using PortIndex  = QtNodes::PortIndex;
    using NodeDataType = QtNodes::NodeDataType;
    using NodeData     = std::shared_ptr<QtNodes::NodeData>;
    using ConnectionPolicy = QtNodes::ConnectionPolicy;
    using ConnectionId     = QtNodes::ConnectionId;

    void setNodeFlag(NodeFlag flag, bool enable = true);

    NodeFlags nodeFlags() const { return m_flags; }

    gt::ig::NodeId id() const { return gt::ig::fromInt(m_id); }

    gt::ig::Position pos() const { return { m_posX, m_posY }; }

    void setId(gt::ig::NodeId id);

    void setPos(QPointF pos);

    void updateObjectName();

    bool isValid() const;

    bool isValid(QString const& modelName);

    /* model specifc methods */

    QString caption() const { return m_caption; }

    void setCaption(QString caption) { m_caption = std::move(caption); }

    QString modelName() const { return metaObject()->className(); }

    virtual unsigned nPorts(PortType type) const { return 0; }

    virtual NodeDataType dataType(PortType type, PortIndex idx) const { return {}; }

    virtual bool portCaptionVisible(PortType type, PortIndex idx) const { return false; }

    virtual QString portCaption(PortType type, PortIndex idx) const { return {}; }

    virtual ConnectionPolicy portConnectionPolicy(PortType type, PortIndex idx) const
    {
        return (type == PortType::In) ? ConnectionPolicy::One :
                                        ConnectionPolicy::Many;
    }

    virtual NodeData outData(PortIndex const port) { return {}; }

    virtual void setInData(NodeData data, PortIndex const port) { }

    virtual QWidget* embeddedWidget() { return nullptr; }

    /* serialization */

    static std::unique_ptr<GtIntelliGraphNode> fromJson(QJsonObject const& json) noexcept(false);

    QJsonObject toJson() const;

public slots:

    virtual void updateNode() {};

signals:

    void dataUpdated(GtIntelliGraphNode::PortIndex const idx);

    void dataInvalidated(GtIntelliGraphNode::PortIndex const idx);

    void computingStarted();

    void computingFinished();

    void embeddedWidgetSizeUpdated();

    void portsAboutToBeDeleted(GtIntelliGraphNode::PortType const type,
                               GtIntelliGraphNode::PortIndex const first,
                               GtIntelliGraphNode::PortIndex const last);

    void portsDeleted();

    void portsAboutToBeInserted(GtIntelliGraphNode::PortType const type,
                                GtIntelliGraphNode::PortIndex const first,
                                GtIntelliGraphNode::PortIndex const last);

    void portsInserted();

protected:

    GtIntelliGraphNode(QString const& caption, GtObject* parent = nullptr);

protected slots:

    virtual void inputConnectionCreated(GtIntelliGraphNode::ConnectionId const& id) {}

    virtual void inputConnectionDeleted(GtIntelliGraphNode::ConnectionId const& id) {}

    virtual void outputConnectionCreated(GtIntelliGraphNode::ConnectionId const& id) {}

    virtual void outputConnectionDeleted(GtIntelliGraphNode::ConnectionId const & id) {}

private:

    /// node id
    GtIntProperty m_id;
    /// x position of node
    GtDoubleProperty m_posX;
    /// y position of node
    GtDoubleProperty m_posY;

    QString m_caption;

    NodeFlags m_flags = gt::ig::None;

    /// will attempt to load and merge memento from json
    void mergeJsonMemento(QJsonObject const& internals);

    /// will write memento data to json
    void toJsonMemento(QJsonObject& internals) const;
};

#endif // GT_INTELLIGRAPHNODE_H
