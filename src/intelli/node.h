/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_NODE_H
#define GT_INTELLI_NODE_H

#include <intelli/globals.h>
#include <intelli/exports.h>

#include <gt_object.h>

#include <QWidget>

namespace intelli
{

enum NodeFlag
{
    NoFlag      = 0,
    /// Indicates node is resizeable
    Resizable   = 1,
    /// Indicates node caption should be hidden
    HideCaption = 2,
    /// Indicates node is unique (i.e. only one instance should exist)
    Unique      = 4
};

using NodeFlags  = int;

class NodeExecutor;
class NodeData;
class GraphExecutionModel;
struct NodeImpl;

/**
 * @brief Creates a base widget that has a simple layout attached. Can be used
 * for widgets, that have trouble resizing correctly.
 * @return Widget pointer (never null)
 */
GT_INTELLI_EXPORT std::unique_ptr<QWidget> makeWidget();

class GT_INTELLI_EXPORT Node : public GtObject
{
    Q_OBJECT
    
    friend class NodeExecutor;
    friend class GraphExecutionModel;

public:

    using NodeId      = intelli::NodeId;
    using NodeFlag    = intelli::NodeFlag;
    using NodeFlags   = intelli::NodeFlags;
    using PortType    = intelli::PortType;
    using PortId      = intelli::PortId;
    using PortIndex   = intelli::PortIndex;
    using Position    = intelli::Position;
    using NodeDataPtr = intelli::NodeDataPtr;

    /// widget factory function type. Parameter is guranteed to be of type
    /// "this" and can be casted safely using static_cast.
    using WidgetFactory =
        std::function<std::unique_ptr<QWidget>(Node& thisNode)>;
    using WidgetFactoryNoArgs =
        std::function<std::unique_ptr<QWidget>()>;

    /// enum for defining whether a port is optional
    enum PortPolicy
    {
        Required,
        Optional,
        DefaultPortPolicy = Optional
    };

    /// port data struct
    class PortData
    {
    public:

        // cppcheck-suppress noExplicitConstructor
        PortData(QString _typeId) : PortData(std::move(_typeId), {}) {}

        PortData(QString _typeId,
                 QString _caption,
                 bool _captionVisible = true,
                 bool _optional = true) :
            typeId(std::move(_typeId)),
            caption(std::move(_caption)),
            captionVisible(_captionVisible),
            optional(_optional)
        {}

        // type id for port data (classname)
        QString typeId;
        // custom port caption (optional)
        QString caption;
        // whether port caption should be visible
        bool captionVisible = true;
        // whether the port is required for the node evaluation
        bool optional = true;

        /**
         * @brief Returns the port id
         * @return port id
         */
        inline PortId id() const { return m_id; }

    private:

        PortId m_id{};
        
        friend class Node;
    };
    
    ~Node();

    /**
     * @brief Setter for the automatic node evaluation flag
     * @param active Whether to enable automatic node evaluation
     */
    void setActive(bool active = true);

    /**
     * @brief Returns true if automatic node evaluation is active
     * @return is active
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
    void setPos(Position pos);

    /**
     * @brief pos
     * @return
     */
    Position pos() const;

    /**
     * @brief Sets size property. Does not trigger a resize of the widget
     * @param pos new position
     */
    void setSize(QSize size);

    /**
     * @brief size
     * @return
     */
    QSize size() const;

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
     * @brief Returns the node flags
     * @return node flags
     */
    NodeFlags nodeFlags() const;

    /**
     * @brief Setter for the caption. Will be saved persistently
     * @param caption New caption
     * @return Returns a reference to this node for method chaining
     */
    Node& setCaption(QString const& caption);

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
     * @return Port index. May be invalid, check using "invalid<PortIndex>()"
     */
    PortIndex portIndex(PortType type, PortId id) const noexcept(false);

    /**
     * @brief Attempts to find the port id by port index and the port type.
     * @param type Port type (input or output)
     * @param idx Port index
     * @return Port id. May be invalid, check using "invalid<PortId>()"
     */
    PortId portId(PortType type, PortIndex idx) const noexcept(false);

    /**
     * @brief Returns the embedded widget used in the intelli graph. Ownership
     * may be transfered safely. Note: Will instantiate the widget if it does
     * not yet exists
     * @return Embedded widget
     */
    QWidget* embeddedWidget();

signals:

    /**
     * @brief Triggers the evaluation of the output port speicified. It is not
     * guranteed to be evaluated, as the underling graph execution model must
     * be active
     * @param portId Port id to evaluate. If port id is invalid, the whole
     * node (i.e. all ports) should be evaluated
     */
    void triggerPortEvaluation(PortId portId);

    /**
     * @brief Helper signal to trigger the evaluation of the whole node.
     */
    void triggerNodeEvaluation();

    /**
     * @brief Emitted if the node has evaluated and the output data has changed.
     * Will be called automatically and should not be triggered by the "user".
     * @param portId Output port that was evaluated
     */
    void evaluated(PortId portId = invalid<PortId>());

    /**
     * @brief Emitted if new input data was recieved, just before evaluating.
     * Data may be invalid. Should not be triggered by the "user".
     * @param portId Input port that recieved data
     */
    void inputDataRecieved(PortId portId = invalid<PortId>());

    /**
     * @brief Emitted once the node evaluation has started
     */
    void computingStarted();
    
    /**
     * @brief Emitted once the node evaluation has finished
     */
    void computingFinished();

    /**
     * @brief stateChanged
     */
    void nodeStateChanged();

    /**
     * @brief Emitted if node specific data has changed (cpation, number of
     * ports etc.). May be invoked by the "user" to update the graphical node
     * representation in case a port has changed for example.
     */
    void nodeChanged();

    /**
     * @brief Emitted if port specific data has changed (e.g. port cpation).
     * May be invoked by the "user" to update the graphical node representation
     * @param id id of port that changed
     */
    void portChanged(PortId id);

    /**
     * @brief Will be called internally before deleting a port.
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
     * @brief Will be called internally before inserting a port.
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

    /**
     * @brief Will be emitted once a port was connected
     * @param type Port type (i.e. Input or Output port)
     * @param idx Port index that was connected
     */
    void portConnected(PortType type, PortIndex idx);

    /**
     * @brief Will be emitted once a port was disconnected
     * @param type Port type (i.e. Input or Output port)
     * @param idx Port index that was disconnected
     */
    void portDisconnected(PortType type, PortIndex idx);

protected:

    /// prefer Graph::appendNode
    using GtObject::setParent;

    /**
     * @brief constructor. Must initialize the model name.
     * @param modelName Model name. May not be altered later
     * @param parent Parent object
     */
    Node(QString const& modelName, GtObject* parent = nullptr);

    /**
     * @brief Main evaluation method to override. Will be called for each output
     * port. If no output ports are registered, but input ports are, an invalid
     * port id will be passed and the returned data will be discarded. Will not
     * be called if any required input port has no valid data associated
     * (see PortPolicy)
     * @param outId Output port id to evaluate the data for
     * @return Node data on the output port
     */
    virtual NodeDataPtr eval(PortId outId);

    GraphExecutionModel* executionModel();
    GraphExecutionModel const* executionModel() const;

    /**
     * @brief Handles the evaluation of the node (port). It is not intended to
     * actually do the evaluation (use `eval` instead), but to handle/manage the
     * execution of the node. Should only be overriden in rare cases.
     * @param portId Port id to evaluate. If port id is invalid, the whole
     * node (i.e. all ports) should be evaluated
     * @return Returns true if the evaluation was triggered sucessfully.
     * (node may be evaluated non-blocking)
     */
    virtual bool handleNodeEvaluation(GraphExecutionModel& model, PortId portId);

    /**
     * @brief Should be called within the constructor. Used to register
     * the widget factory, used for creating the embedded widget within the
     * intelli graphs.
     * @param factory Widget factory
     */
    void registerWidgetFactory(WidgetFactory factory);
    void registerWidgetFactory(WidgetFactoryNoArgs factory);

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
    PortId addInPort(PortData port, PortPolicy policy = DefaultPortPolicy) noexcept(false);

    /**
     * @brief Appends the output port
     * @param port Port data to append
     * @return Port id
     */
    PortId addOutPort(PortData port) noexcept(false);

    /**
     * @brief Inserts an input port at the given location
     * (-1 will append to back)
     * @param port Port data to insert
     * @param idx Where to insert the port
     * @param policy Input port policy
     * @return Port id
     */
    PortId insertInPort(PortData port, int idx, PortPolicy policy = DefaultPortPolicy) noexcept(false);

    /**
     * @brief Inserts an output port at the given location
     * (-1 will append to back)
     * @param port Port data to insert
     * @param idx Where to insert the port
     * @return Port id
     */
    PortId insertOutPort(PortData port, int idx) noexcept(false);

    /**
     * @brief Helper method for inserting a port. Prefer the explicit insert/add
     * methods.
     * @param type Whether to insert an in or out port
     * @param port Port data to insert
     * @param idx Where to insert the port
     * @return Port id
    */
    PortId insertPort(PortType type, PortData port, int idx = -1) noexcept(false);

    /**
     * @brief Removes the port specified by id
     * @param id Port to remove
     * @return Success
     */
    bool removePort(PortId id);

    /**
     * @brief Returns the node data of the specified port
     * @param id Port id (output or input)
     * @return Port data (may be null)
     */
    NodeDataPtr nodeData(PortId id) const;

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

private:

    std::unique_ptr<NodeImpl> pimpl;

    // hide object name setter
    using GtObject::setObjectName;

    /// initializes the widgets
    void initWidget();
};

} // namespace intelli

using GtIntelliGraphNode [[deprecated]] = intelli::Node;

inline gt::log::Stream&
operator<<(gt::log::Stream& s, intelli::Node::PortData const& d)
{
    {
        gt::log::StreamStateSaver saver(s);
        s.nospace()
            << "PortData[" << d.typeId << "/" << d.id() << "]";
    }
    return s.doLogSpace();
}

#endif // GT_INTELLI_NODE_H
