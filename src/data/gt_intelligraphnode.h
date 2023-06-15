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
#include "gt_igvolatileptr.h"
#include "gt_intelligraph_exports.h"
#include "gt_ignodedata.h"

#include "gt_object.h"
#include "gt_intproperty.h"
#include "gt_doubleproperty.h"

#include <QWidget>

namespace gt
{
namespace ig
{

enum NodeFlag
{
    NoFlag = 0x0,
    Resizable = 0x1,
    HideCaption = 0x4,
};

using NodeFlags  = int;

} // namespace ig

} // namespace gt

class GT_IG_EXPORT GtIntelliGraphNode : public GtObject
{
    Q_OBJECT

public:

    using NodeId     = gt::ig::NodeId;
    using NodeFlag   = gt::ig::NodeFlag;
    using NodeFlags  = gt::ig::NodeFlags;
    using PortType   = gt::ig::PortType;
    using PortId     = gt::ig::PortId;
    using PortIndex  = gt::ig::PortIndex;
    using Position   = gt::ig::Position;

    using NodeData = std::shared_ptr<const GtIgNodeData>;

    /// widget factory function type. Parameter is guranteed to be of type
    /// "this" and can be casted safely using static_cast.
    using WidgetFactory =
        std::function<std::unique_ptr<QWidget>(GtIntelliGraphNode& thisNode)>;

    /// state enum
    enum State
    {
        EvalRequired = 0,
        Evaluated,
        Evaluating
    };

    /// enum for defining whether a port is optional
    enum PortPolicy
    {
        Required,
        Optional
    };

    /// port data struct
    class PortData
    {
    public:

        // cppcheck-suppress noExplicitConstructor
        PortData(QString _typeId) : PortData(std::move(_typeId), {}) {}

        PortData(QString _typeId, QString _caption, bool _captionVisible = false) :
            typeId(std::move(_typeId)),
            caption(std::move(_caption)),
            captionVisible(_captionVisible),
            optional(true)
        {}

        // type id for port data (classname)
        QString typeId;
        // optional custom caption
        QString caption;
         // whether caption is visible
        bool captionVisible;
        // whether the port is required for the node evaluation
        bool optional;

        inline PortId id() const { return m_id; }

    private:

        PortId m_id = gt::ig::invalid<PortId>();

        friend class GtIntelliGraphNode;
    };

    /* node specifc methods */

    NodeId id() const { return gt::ig::fromInt(m_id); }

    Position pos() const { return { m_posX, m_posY }; }

    void setId(NodeId id);

    void setPos(QPointF pos);

    void updateObjectName();

    bool isValid() const;

    bool isValid(QString const& modelName);

    NodeFlags nodeFlags() const { return m_flags; }

    void setCaption(QString caption);

    QString const& caption() const;

    QString const& modelName() const;

    std::vector<PortData> const& ports(PortType type) const noexcept(false);

    PortData const* port(PortId id) const noexcept;

    PortData const* port(PortType type, PortIndex idx) const noexcept;

    PortIndex portIndex(PortType type, PortId id) const noexcept(false);

    PortId portId(PortType type, PortIndex idx) const noexcept(false);

    QWidget* embeddedWidget();

    void setInData(PortIndex idx, NodeData data);

    NodeData outData(PortIndex idx);

    /* serialization */

    static std::unique_ptr<GtIntelliGraphNode> fromJson(QJsonObject const& json
                                                        ) noexcept(false);

    QJsonObject toJson() const;

    /// will attempt to load and merge memento from json
    void mergeJsonMemento(QJsonObject const& internals);

    /// will write memento data to json
    void toJsonMemento(QJsonObject& internals) const;

public slots:

    void updateNode();

    void updatePort(gt::ig::PortIndex idx);

signals:

    void outDataUpdated(gt::ig::PortIndex const idx = 0);

    void outDataInvalidated(gt::ig::PortIndex const idx = 0);

    void inputDataRecieved(gt::ig::PortIndex const idx = 0);

protected:

    /**
     * @brief GtIntelliGraphNode
     * @param caption
     * @param parent
     */
    GtIntelliGraphNode(QString const& caption, GtObject* parent = nullptr);

    virtual NodeData eval(PortId outId);

    /**
     * @brief Should be called within the constructor. Used to register
     * the widget factory, used for creating the embedded widget within the
     * intelli graphs.
     * @param factory Widget factory
     */
    void registerWidgetFactory(WidgetFactory factory);

    void setNodeFlag(NodeFlag flag, bool enable = true);

    void setModelName(QString name);

    PortId addInPort(PortData port, PortPolicy policy = PortPolicy::Required) noexcept(false);
    PortId addOutPort(PortData port) noexcept(false);

    PortId insertInPort(PortData port, int idx, PortPolicy policy = PortPolicy::Required) noexcept(false);
    PortId insertOutPort(PortData port, int idx) noexcept(false);

    bool removePort(PortId id);

    GtIgNodeData const* portData(PortId inId) const;

private:

    /// node id
    GtIntProperty m_id;
    /// x position of node
    GtDoubleProperty m_posX;
    /// y position of node
    GtDoubleProperty m_posY;
    /// model name string
    QString m_modelName;
    /// caption string
    QString m_caption;
    /// ports
    std::vector<PortData> m_inPorts, m_outPorts;

    std::vector<NodeData> m_inData, m_outData;
    /// owning pointer to widget, may be deleted earlier
    gt::ig::volatile_ptr<QWidget> m_widget;
    /// factory for creating the widget
    WidgetFactory m_widgetFactory;
    /// node flags
    NodeFlags m_flags = gt::ig::None;
    /// iterator for the next port id
    PortId m_nextPortId = 0;

    State m_state = EvalRequired;

    /// initialzes the widget
    void initWidget();

    bool canEvaluate() const;

    PortId insertPort(PortType type, PortData port, int idx = -1) noexcept(false);

    /// non const overload for internal use only
    std::vector<PortData>& ports_(PortType type) noexcept(false);
    std::vector<NodeData>& portData_(PortType type) noexcept(false);
};

inline gt::log::Stream&
operator<<(gt::log::Stream& s, GtIntelliGraphNode::PortData const& d)
{
    {
        gt::log::StreamStateSaver saver(s);
        s.nospace()
            << "PortData[" << d.typeId << "]";
    }
    return s;
}

#endif // GT_INTELLIGRAPHNODE_H
