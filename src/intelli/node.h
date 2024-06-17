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
    /// Indicates node caption should be hidden
    HideCaption = 1 << 1,
    /// Indicates node is unique (i.e. only one instance should exist)
    Unique      = 1 << 2,

    /// Indicates that the widget should be placed so that its size can be maximized
    MaximizeWidget = 1 << 3,
    /// Indicates node is resizeable
    Resizable   = 1 << 4,
    /// Indicates node is only resizeable horizontally
    ResizableHOnly = 1 << 5,

    /// Indicates that the node is evaluating (will be set automatically)
    Evaluating  = 1 << 7,

    /// default node flags
    DefaultNodeFlags = NoFlag,

    /// mask to check if node is resizable
    IsResizableMask = Resizable | ResizableHOnly
};

using NodeFlags = unsigned int;

enum class NodeEvalMode
{
    /// Indicates that the node will be evaluated non blockingly in a separate
    /// thread
    Detached = 0,
    /// Indicates the the node should be evaluated exclusively to other nodes in
    /// a separate thread
    Exclusive,
    /// Indicates that the node should be evaluated in the main thread, thus
    /// blocking the GUI. Should only be used if node evaluates instantly.
    MainThread,
    /// Default behaviour
    Default = Detached,
};

class NodeExecutor;
class NodeData;
class NodeDataInterface;
class GraphExecutionModel;
struct NodeImpl;

/**
 * @brief Creates a base widget that has a simple layout attached. Can be used
 * for widgets, that have trouble resizing correctly.
 * @return Widget pointer (never null)
 */
GT_INTELLI_EXPORT std::unique_ptr<QWidget> makeBaseWidget();

class GT_INTELLI_EXPORT Node : public GtObject
{
    Q_OBJECT
    
    friend class NodeExecutor;
    friend class NodeGraphicsObject;
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
    class PortInfo
    {
    public:

        // cppcheck-suppress noExplicitConstructor
        PortInfo(QString _typeId) : PortInfo(std::move(_typeId), {}) {}

        PortInfo(QString _typeId,
                 QString _caption,
                 bool _captionVisible = true,
                 bool _optional = true) :
            typeId(std::move(_typeId)),
            caption(std::move(_caption) ),
            captionVisible(_captionVisible),
            optional(_optional)
        {}

        /// creates a PortInfo struct with a custom port id
        template<typename ...T>
        static PortInfo customId(PortId id, T&&... args)
        {
            PortInfo pd(std::forward<T>(args)...);
            pd.m_id = id;
            return pd;
        }

        /// creates a copy of this object but resets the id parameter
        PortInfo copy() const
        {
            PortInfo pd(*this);
            pd.m_id = invalid<PortId>();
            return pd;
        }

        // type id for port data (classname)
        QString typeId;
        // custom port caption (optional)
        QString caption;
        // whether port caption should be visible
        bool captionVisible = true;
        // whether the port is visible at all
        bool visible = true;
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
    using PortData [[deprecated("use PortInfo")]] = PortInfo;
    
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
    Node& setPos(Position pos);

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
     * @return Node flags
     */
    NodeFlags nodeFlags() const;

    /**
     * @brief Return the node eval mode
     * @return Node eval mode
     */
    NodeEvalMode nodeEvalMode() const;

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
    std::vector<PortInfo> const& ports(PortType type) const noexcept(false);

    /**
     * @brief Returns the port for the port id
     * @param id Port id
     * @return Port. May be null
     */
    PortInfo* port(PortId id) noexcept;
    PortInfo const* port(PortId id) const noexcept;

    /**
     * @brief Returns the port index of the port id and its port type.
     * @param type Port type (input or output)
     * @param id Port id
     * @return Port index. May be invalid, check using `invalid<PortIndex>()`
     */
    PortIndex portIndex(PortType type, PortId id) const noexcept(false);

    /**
     * @brief Returns the port type of the given port
     * @param id Port id
     * @return Port type. `PortyType::NoTyp` is returned if the given port
     * does not exist
     */
    PortType portType(PortId id) const noexcept(false);

    /**
     * @brief Attempts to find the port id by port index and the port type.
     * @param type Port type (input or output)
     * @param idx Port index
     * @return Port id. May be invalid, check using `invalid<PortId>()`
     */
    PortId portId(PortType type, PortIndex idx) const noexcept(false);

    /**
     * @brief Returns whether a port is connected
     * @param id Port id to check
     * @return Is port connected
     */
    bool isPortConnected(PortId portId) const;

signals:

    /**
     * @brief Triggers the evaluation of node. It is not guranteed to be
     * evaluated, as the underling graph execution model must be active
     */
    void triggerNodeEvaluation();

    /**
     * @brief Emitted if the node has evaluated and the output data has changed.
     * Will be called automatically and should not be triggered by the "user".
     */
    void evaluated();

    /**
     * @brief Emitted if new input data was recieved, just before evaluating.
     * Data may be invalid. Should not be triggered by the "user".
     * @param portId Input port that recieved data
     */
    void inputDataRecieved(PortId portId = invalid<PortId>());

    /**
     * @brief Emitted once the node evaluation has started. Will update the node
     * flags `RequiresEvaluation` and `Evaluating` automatically.
     */
    void computingStarted();
    
    /**
     * @brief Emitted once the node evaluation has finished. Will update the
     * node flag `Evaluating` automatically.
     */
    void computingFinished();

    /**
     * @brief Emitted if node specific data has changed (cpation, number of
     * ports etc.). May be invoked by the "user" to update the graphical node
     * representation in case a port has changed for example.
     */
    void nodeChanged();

    /**
     * @brief Emitted just before the node is deleted similar to
     * QObject::destroyed with the difference that one may still access members
     */
    void nodeAboutToBeDeleted(NodeId nodeId);

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
     * @param id Port that was connected
     */
    void portConnected(PortId id);

    /**
     * @brief Will be emitted once a port was disconnected
     * @param id Port that was disconnected
     */
    void portDisconnected(PortId id);

    void isActiveChanged();

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
     * @brief Main evaluation method to override. Will be called once, the
     * "user" has to calculate and set the data for all output ports. Will not
     * be called if any required input port has no valid data associated
     * (see PortPolicy)
     */
    virtual void eval();

    /**
     * @brief Handles the evaluation of the node. This method is not intended to
     * actually do the evaluation (use `eval` instead), but to handle/manage the
     * execution of the node. Should only be overriden in rare cases.
     * Note: When overriding do not forget to emit the `computingStarted` and
     * `computingFinished` respectively.
     * @return Returns true if the evaluation was triggered sucessfully.
     * (node may be evaluated non-blocking)
     */
    virtual bool handleNodeEvaluation(GraphExecutionModel& model);

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
     * @brief Sets the node evaluation mode
     * @param mode Node eval mode
     */
    void setNodeEvalMode(NodeEvalMode mode);

    /**
     * @brief Appends the output port
     * @param port Port data to append
     * @param policy Input port policy
     * @return Port id
     */
    PortId addInPort(PortInfo port, PortPolicy policy = DefaultPortPolicy) noexcept(false);

    /**
     * @brief Appends the output port
     * @param port Port data to append
     * @return Port id
     */
    PortId addOutPort(PortInfo port) noexcept(false);

    /**
     * @brief Inserts an input port at the given location
     * (-1 will append to back)
     * @param port Port data to insert
     * @param idx Where to insert the port
     * @param policy Input port policy
     * @return Port id
     */
    PortId insertInPort(PortInfo port, int idx, PortPolicy policy = DefaultPortPolicy) noexcept(false);

    /**
     * @brief Inserts an output port at the given location
     * (-1 will append to back)
     * @param port Port data to insert
     * @param idx Where to insert the port
     * @return Port id
     */
    PortId insertOutPort(PortInfo port, int idx) noexcept(false);

    /**
     * @brief Helper method for inserting a port. Prefer the explicit insert/add
     * methods.
     * @param type Whether to insert an in or out port
     * @param port Port data to insert
     * @param idx Where to insert the port
     * @return Port id
    */
    PortId insertPort(PortType type, PortInfo port, int idx = -1) noexcept(false);

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
     * @brief Sets the node data at the specified port. Should be used
     * inside the eval method.
     * @param id Port to set the data of
     * @param data The new data
     * @return Success
     */
    bool setNodeData(PortId id, NodeDataPtr data);

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
};

} // namespace intelli

namespace gt
{
namespace log
{

inline Stream&
operator<<(Stream& s, intelli::Node::PortInfo const& p)
{
    {
        StreamStateSaver saver(s);
        s.nospace()
            << "Port[" << p.typeId << "/" << p.id() << "]";
    }
    return s.doLogSpace();
}

} // namespace log

} // namespace gt


#endif // GT_INTELLI_NODE_H
