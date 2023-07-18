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
#include "gt_intelligraph_exports.h"
#include "gt_ignodedata.h"

#include "gt_object.h"

#include <QWidget>

namespace gt
{
namespace ig
{

enum NodeFlag
{
    NoFlag      = 0x0,
    // Indicates node is resizeable
    Resizable   = 0x1,
    // Indicates node caption should be hidden
    HideCaption = 0x2,
    // Indicates node is unique (i.e. only one instance should exist)
    Unique      = 0x4
};

using NodeFlags  = int;

} // namespace ig

} // namespace gt

class GT_IG_EXPORT GtIntelliGraphNode : public GtObject
{
    Q_OBJECT

public:

    using NodeId    = gt::ig::NodeId;
    using NodeFlag  = gt::ig::NodeFlag;
    using NodeFlags = gt::ig::NodeFlags;
    using PortType  = gt::ig::PortType;
    using PortId    = gt::ig::PortId;
    using PortIndex = gt::ig::PortIndex;
    using Position  = gt::ig::Position;

    using NodeData = std::shared_ptr<const GtIgNodeData>;

    /// widget factory function type. Parameter is guranteed to be of type
    /// "this" and can be casted safely using static_cast.
    using WidgetFactory =
        std::function<std::unique_ptr<QWidget>(GtIntelliGraphNode& thisNode)>;

    /// state enum
    enum State
    {
        EvalRequired = 0,
        Evaluated    = 1,
        Evaluating   = 2
    };

    /// enum for defining whether a port is optional
    enum PortPolicy
    {
        Required,
        Optional,
        DoNotEvaluate
    };

    /// port data struct
    class PortData
    {
    public:

        // cppcheck-suppress noExplicitConstructor
        PortData(QString _typeId) : PortData(std::move(_typeId), {}) {}

        PortData(QString _typeId, QString _caption, bool _captionVisible = true) :
            typeId(std::move(_typeId)),
            caption(std::move(_caption)),
            captionVisible(_captionVisible)
        {}

        // type id for port data (classname)
        QString typeId;
        // custom port caption (optional)
        QString caption;
        // whether port caption should be visible
        bool captionVisible;
        // whether the port is required for the node evaluation
        bool optional = true;

        bool evaluate = true;

        /**
         * @brief Returns the port id
         * @return port id
         */
        inline PortId id() const { return m_id; }

    private:

        PortId m_id{};

        friend class GtIntelliGraphNode;
    };

    ~GtIntelliGraphNode();

    /* node specifc methods */

    /**
     * @brief Sets the node active or disables it. Only an active node can be
     * evaluated. A node is deactivated by default to evaluate only if necessary
     * @param isActive Whether the node should be (de-)activated
     */
    void setActive(bool isActive = true);

    /**
     * @brief Returns whether the node is active. Only an active node can be
     * evaluated.
     * @return Is active
     */
    bool isActive() const;

    /**
     * @brief Sets the node id. handle with care, as this may result in
     * undesired behaviour. Will be saved persistently.
     * @param id New id
     */
    void setId(NodeId id);

    /**
     * @brief Returns the node id
     * @return Node id
     */
    NodeId id() const;

    /**
     * @brief Sets the new node position. Will be saved persistently.
     * @param pos new position
     */
    void setPos(QPointF pos);

    /**
     * @brief pos
     * @return
     */
    Position pos() const;

    /**
     * @brief Will create a unique object name based on the node caption
     */
    void updateObjectName();

    /**
     * @brief Returns true if the node (id) is valid
     * @return is valid
     */
    bool isValid() const;

    /**
     * @brief Returns whether the node is valid and has the expected model name
     * @param modelName Model anme
     * @return is valid
     */
    bool isValid(QString const& modelName);

    /**
     * @brief Returns the node flags
     * @return node flags
     */
    NodeFlags nodeFlags() const;

    /**
     * @brief Setter for the caption. Will be saved persistently
     * @param caption New caption
     */
    void setCaption(QString const& caption);

    /**
     * @brief Caption of the node
     * @return Caption
     */
    QString caption() const;

    /**
     * @brief Returns the object name, that does not contain any symbols or
     * enumerations to make it unique.
     *
     * Example:
     * #1 "My Fancy Node" -> "My Fancy Node"
     * #2 "My Fancy Node[2]" -> "My Fancy Node"
     *
     * @return Base object name
     */
    QString baseObjectName() const;

    /**
     * @brief Model name of the node
     * @return Model name
     */
    QString const& modelName() const;

    /**
     * @brief Returns a list of the input or output ports depending on the port
     * type
     * @param type Port type (input or output)
     * @return Ports
     */
    std::vector<PortData> const& ports(PortType type) const noexcept(false);

    /**
     * @brief Returns the port for the port id
     * @param id Port id
     * @return Port. May be null
     */
    PortData* port(PortId id) noexcept;
    PortData const* port(PortId id) const noexcept;

    /**
     * @brief Returns the port index for the port id and the port type.
     * @param type Port type (input or output)
     * @param id Port id
     * @return Port index. May be invalid, check using "gt::ig::invalid<PortIndex>()"
     */
    PortIndex portIndex(PortType type, PortId id) const noexcept(false);

    /**
     * @brief Attempts to find the port id by port index and the port type.
     * @param type Port type (input or output)
     * @param idx Port index
     * @return Port id. May be invalid, check using "gt::ig::invalid<PortId>()"
     */
    PortId portId(PortType type, PortIndex idx) const noexcept(false);

    /**
     * @brief Returns the embedded widget used in the intelli graph. Ownership
     * may be transfered safely. Note: Will instantiate the widget if it does
     * not yet exists
     * @return Embedded widget
     */
    QWidget* embeddedWidget();

    /**
     * @brief Sets the node data at the input port specified by the index.
     * Triggers the evaluation of all output ports
     * @param idx Input port index
     * @param data Node data.
     */
    bool setInData(PortIndex idx, NodeData data);

    /**
     * @brief Returns the output node data specified by the index.
     * @param idx Output port index
     * @return Node data. Null if idx is invalid
     */
    NodeData outData(PortIndex idx);

public slots:

    /**
     * @brief This will schedule the evaluation of all output ports.
     */
    void updateNode();

    /**
     * @brief This will schedule the evaluation of the output port specified by
     * idx.
     * @param idx Output port index. May be calculated using an output port id.
     */
    void updatePort(gt::ig::PortIndex idx);

signals:

    /**
     * @brief Emitted if the node has evaluated and the output data has changed.
     * Will be called automatically and should not be triggered by the "user".
     * @param idx Output port index. May be mapped to an output port id.
     */
    void evaluated(gt::ig::PortIndex idx = PortIndex{0});

    /**
     * @brief Emitted if the output data has changed (may be invalid), just
     * after evaluating. Will be called automatically and should not be triggered
     * by the "user". Triggers the evaluation of all connected ports.
     * @param idx Output port index. May be mapped to an output port id.
     */
    void outDataUpdated(gt::ig::PortIndex idx = PortIndex{0});

    /**
     * @brief Emitted if the output data was invalidated, just after evaluating.
     * Will be called automatically and should not be triggered by the "user".
     * Triggers the invalidation of all connected ports
     * @param idx Output port index. May be mapped to an output port id.
     */
    void outDataInvalidated(gt::ig::PortIndex idx = PortIndex{0});

    /**
     * @brief Emitted if new input data was recieved, just before evaluating.
     * Data may be invalid. Should not be triggered by the "user".
     * @param idx Input port index. May be mapped to an input port id.
     */
    void inputDataRecieved(gt::ig::PortIndex idx = PortIndex{0});

    /**
     * @brief Emitted if node specific data hast changed (cpation, number of
     * ports etc.). May be invoked by the "user" to update the graphical node
     * in case a port hast changed for example.
     */
    void nodeChanged();

    /**
     * @brief May be emitted if the port data changes (e.g. port caption)
     */
    void portChanged(PortId id);

    /**
     * @brief Will be called internally before deleting a point.
     * @param type Port type (input or output)
     * @param idx Affected index
     */
    void portAboutToBeDeleted(PortType type, PortIndex idx);

    /**
     * @brief Will be emiited just after a port was deleted
     * @param type Port type (input or output)
     * @param idx Old index
     */
    void portDeleted(PortType type, PortIndex idx);

    /**
     * @brief Will be called internally before inserting a point.
     * @param type Port type (input or output)
     * @param idx Affected index
     */
    void portAboutToBeInserted(PortType type, PortIndex idx);

     /**
     * @brief Will be emitted just after a port was inserted
     * @param type Port type (input or output)
     * @param idx New index
     */
    void portInserted(PortType type, PortIndex idx);

protected:

    /**
     * @brief constructor. Must initialize the model name.
     * @param modelName Model name. May not be altered later
     * @param parent Parent object
     */
    GtIntelliGraphNode(QString const& modelName, GtObject* parent = nullptr);

    /**
     * @brief Main evaluation method to override. Will be called for each output
     * port. If no output ports are registered, but input ports are, an invalid
     * port id will be passed and the returned data will be discarded. Will not
     * be called if any required input port has no valid data associated
     * (see PortPolicy)
     * @param outId Output port id to evaluate the data for
     * @return Node data on the output port
     */
    virtual NodeData eval(PortId outId);

    /**
     * @brief Should be called within the constructor. Used to register
     * the widget factory, used for creating the embedded widget within the
     * intelli graphs.
     * @param factory Widget factory
     */
    void registerWidgetFactory(WidgetFactory factory);

    /**
     * @brief Sets a node flag
     * @param flag Flag to set
     * @param enable Whether to enable or disable the flag
     */
    void setNodeFlag(NodeFlag flag, bool enable = true);

    /**
     * @brief Appends the output port
     * @param port Port data to append
     * @param policy Input port policy
     * @return Port id
     */
    PortId addInPort(PortData port, PortPolicy policy = PortPolicy::Optional) noexcept(false);
    /**
     * @brief Appends the output port
     * @param port Port data to append
     * @return Port id
     */
    PortId addOutPort(PortData port, PortPolicy policy = PortPolicy::Optional) noexcept(false);

    /**
     * @brief Inserts an input port at the given location
     * (-1 will append to back)
     * @param port Port data to append
     * @param idx Where to insert the port
     * @param policy Input port policy
     * @return Port id
     */
    PortId insertInPort(PortData port, int idx, PortPolicy policy = PortPolicy::Optional) noexcept(false);
    /**
     * @brief Inserts an output port at the given location
     * (-1 will append to back)
     * @param port Port data to append
     * @param idx Where to insert the port
     * @return Port id
     */
    PortId insertOutPort(PortData port, int idx, PortPolicy policy = PortPolicy::Optional) noexcept(false);

    /**
     * @brief Removes the port specified by id
     * @param id Port to remove
     * @return Success
     */
    bool removePort(PortId id);

    /**
     * @brief Returns the specified port data
     * @param id Port id (output or input)
     * @return Port data (may be null)
     */
    [[deprecated("Use nodeData instead")]]
    NodeData const& portData(PortId id) const { return nodeData(id); }

    /**
     * @brief Overload that casts the port data to the desired type.
     * @param id Port id (output or input)
     * @return Port data
     */
    template <typename T, typename U = std::remove_pointer_t<T>>
    [[deprecated("Use nodeData<T*> instead")]]
    U const* portData(PortId id) const
    {
        return qobject_cast<U const*>(nodeData(id).get());
    }

    /**
     * @brief Returns the node data of the specified port
     * @param id Port id (output or input)
     * @return Port data (may be null)
     */
    NodeData const& nodeData(PortId id) const;

    /**
     * @brief Overload that casts the node data of the specified port to the
     * desired type.
     * @param id Port id (output or input)
     * @return Port data
     */
    template <typename T, typename U = std::remove_pointer_t<T>>
    U const* nodeData(PortId id) const
    {
        return qobject_cast<U const*>(nodeData(id).get());
    }

    void onObjectDataMerged() override { gtDebug() << __FUNCTION__ << objectName(); }

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;

    // hide object name setter
    using GtObject::setObjectName;

    /// initialzes the widgets
    void initWidget();

    /// helper method for inserting a port
    PortId insertPort(PortType type, PortData port, int idx = -1) noexcept(false);

    /// internal use only
    std::vector<PortData>& ports_(PortType type) const noexcept(false);
    /// internal use only
    std::vector<NodeData>& nodeData_(PortType type) const noexcept(false);
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
