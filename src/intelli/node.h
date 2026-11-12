/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NODE_H
#define GT_INTELLI_NODE_H

#include <intelli/exports.h>
#include <intelli/globals.h>
#include <intelli/flags.h>
#include <intelli/nodeport.h>
#include <intelli/nodedata.h>
#include <intelli/view.h>

#include <gt_typetraits.h>
#include <gt_object.h>

#include <QWidget>

namespace intelli
{

class NodeEvent;

enum NodeFlag : unsigned
{
    /// no flag
    NoFlag      = 0,
    /// Indicates node caption should be hidden
    HideCaption = 1 << 1,
    /// Indicates node is unique (i.e. only one instance should exist)
    Unique      = 1 << 2,
    /// Indicates that the widget should be placed so that its size can be maximized
    MaximizeWidget = 1 << 4,
    /// Indicates node is resizeable
    Resizable   = 1 << 5,
    /// Indicates node is only resizeable horizontally
    ResizableHOnly = 1 << 6,
    /// Indicates node is deprecated and should no longer be used
    Deprecated = 1 << 7,

    /// default node flags
    DefaultNodeFlags = NoFlag,

    /// mask to check if node is resizable
    IsResizableMask = Resizable | ResizableHOnly,

    /// base flag for user defined node flags
    UserFlag = 1 << 16
};

using NodeFlags = UFlags<NodeFlag>;

/// mask to check if node should be evaluated in separate thread
constexpr size_t IsDetachedMask = 1 << 0;
/// mask to check if node should be evaluated in main thread
constexpr size_t IsBlockingMask = 1 << 1;
/// mask to check if node should be evaluated exclusively
constexpr size_t IsExclusiveMask = 1 << 2;

enum class NodeEvalMode : unsigned
{
    NoEvaluationRequired = 0,
    /// Indicates that the node will be evaluated non blockingly in a separate
    /// thread
    Detached = IsDetachedMask,
    /// Indicates that the node should be evaluated in the main thread, thus
    /// blocking the GUI. Should only be used if node evaluates instantly.
    Blocking = IsBlockingMask,
    /// Indicates that the node should be evaluated exclusively to other nodes in
    /// a separate thread
    ExclusiveDetached = IsExclusiveMask | IsDetachedMask,
    /// Indicates that the node should be evaluated exclusively to other nodes in
    /// the main thread
    ExclusiveBlocking = IsExclusiveMask | IsBlockingMask,
    /// Indicates that the node has entire control over how it is executed
    Custom = 1 << 3 | IsBlockingMask,
    /// Inidcates that the inputs of the node should be forwarded to the outputs
    /// of the node
    // TODO: remove, only a hack for input output providers
    ForwardInputsToOutputs [[deprecated("Obsolete, use `Blocking` instead")]] = 1 << 4 | IsBlockingMask,
    /// Default behaviour
    Default = Detached,

    /// deprecated
    Exclusive [[deprecated("Use `ExclusiveDetached` or `ExclusiveBlocking` instead")]] = ExclusiveDetached,
    /// deprecated
    MainThread [[deprecated("Use `Blocking` instead")]] = Blocking,
};

class INode;
class Node;
class NodeData;
class NodeDataInterface;
class GraphUserVariables;

namespace exec
{

/**
 * @brief Evaluates the node in the main thread (blocking).A valid node
 * data interface must either be set already or supplied or as an argument.
 * @param node Node to evaluate asyncronously
 * @param model Optional node data interface. If none is supplied the registered
 * node interface ist used (accessed using `nodeDataInterface`).
 * @return success
 */
GT_INTELLI_EXPORT bool blockingEvaluation(Node& node, NodeDataInterface* model = nullptr);

/**
 * @brief Evaluates the node in a separate thread (asyncronously).A valid node
 * data interface must either be set already or supplied or as an argument.
 * @param node Node to evaluate asyncronously
 * @param model Optional node data interface. If none is supplied the registered
 * node interface ist used (accessed using `nodeDataInterface`).
 * @return success
 */
GT_INTELLI_EXPORT bool detachedEvaluation(Node& node, NodeDataInterface* model = nullptr);

/**
 * @brief Triggers the evaluation of the node. Choses whether to evaluate the
 * node asyncronously or blocking according to the node's eval mode. A valid
 * node data interface must either be set already or supplied or as an argument.
 * @param node Node to execute
 * @param model Optional node data interface. If none is supplied the registered
 * node interface ist used (accessed using `nodeDataInterface`).
 * @return success
 */
GT_INTELLI_EXPORT bool triggerNodeEvaluation(Node& node, NodeDataInterface* model = nullptr);

/**
 * @brief Registers a new node data interface for the given node.
 * The node data interface must be set during execution to access input and
 * output data and to set evaluation states.
 * @param node Node to access
 * @param model New node data interface
 */
GT_INTELLI_EXPORT void setNodeDataInterface(Node& node, NodeDataInterface* model);

/**
 * @brief Returns the registered node data interface of the given node.
 * The node data interface must be set during execution to access input and
 * output data and to set evaluation states.
 * @param node Node to access
 * @return Node data interface (may be null)
 */
GT_INTELLI_EXPORT NodeDataInterface* nodeDataInterface(Node& node);

} // namespace exec

/**
 * @brief Posts the node event to `node`
 * @param node Node
 * @param event Node Event to post
 */
GT_INTELLI_EXPORT void postNodeEvent(Node& node, NodeEvent const& event);

/**
 * @brief Creates a base widget that has a simple layout attached. Can be used
 * for widgets, that have trouble resizing correctly.
 * @return Widget pointer (never null)
 */
[[deprecated("Use `utils::makeWidgetWithLayout` instead.")]]
GT_INTELLI_EXPORT std::unique_ptr<QWidget> makeBaseWidget();

class GT_INTELLI_EXPORT Node : public GtObject
{
    Q_OBJECT

    friend class INode;
    friend class NodeUI; // temporary, for accessing widget factory

public:

    struct PortLocation
    {
        PortIndex idx{};
        PortType type{PortType::NoType};

        constexpr bool isValid() const noexcept
        {
            return idx.isValid() && type != PortType::NoType;
        }

        constexpr operator bool() const noexcept
        {
            return isValid();
        }
    };

    using NodeId        = intelli::NodeId;
    using NodeFlag      = intelli::NodeFlag;
    using NodeFlags     = intelli::NodeFlags;
    using NodeEvalMode  = intelli::NodeEvalMode;
    using NodeEvalState = intelli::NodeEvalState;
    using NodePort      = intelli::NodePort;
    using PortInfo      = intelli::NodePort;
    using PortType      = intelli::PortType;
    using PortId        = intelli::PortId;
    using PortIndex     = intelli::PortIndex;
    using Position      = intelli::Position;
    using NodeDataPtr   = intelli::NodeDataPtr;
    using NodeEvent     = intelli::NodeEvent;

    /// widget factory function type. Parameter is guranteed to be of type
    /// "this" and can be casted safely using static_cast.
    using WidgetFactory =
        std::function<std::unique_ptr<QWidget>(Node& thisNode)>;
    using WidgetFactoryNoArgs =
        std::function<std::unique_ptr<QWidget>()>;

    ~Node();

    /// enum for defining whether a port is optional
    enum PortPolicy
    {
        Required,
        Optional,
        DefaultPortPolicy = Optional
    };

    /**
     * @brief Helper method to create a `NodePort` struct given a `TypeId`.
     * @param typeId TypeId
     * @return `NodePort` struct
     */
    static inline NodePort makePort(TypeId typeId)
    {
        return intelli::makePort(std::move(typeId));
    }

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
    NodeId id() const noexcept;

    /**
     * @brief Sets the new node position. Will be saved persistently.
     * @param pos new position
     */
    Node& setPos(Position pos);

    /**
     * @brief pos
     * @return
     */
    Position pos() const noexcept;

    /**
     * @brief Sets size property. Emits `nodeSizeChanged` if size changed.
     * If the flag `NodeFlag::ResizeHOnly` is enabled, only the width is saved.
     * @param size new size
     */
    void setSize(QSize size);

    /**
     * @brief Returns the size of the central widget.
     * If the flag `NodeFlag::ResizeHOnly` is enabled, only the width is
     * returned.
     * @return Size (may be invalid)
     */
    QSize size() const noexcept;
    QSize size(QSize fallback) const noexcept;

    /**
     * @brief Returns true if the node (id) is valid
     * @return is valid
     */
    [[deprecated("Use `id().isValid()` instead")]]
    bool isValid() const { return id().isValid(); }

    /**
     * @brief Returns the node flags
     * @return Node flags
     */
    NodeFlags nodeFlags() const noexcept;

    /**
     * @brief Return the node eval mode
     * @return Node eval mode
     */
    NodeEvalMode nodeEvalMode() const noexcept;

    /**
     * @brief Return the node eval state
     * @return Node eval state
     */
    NodeEvalState nodeEvalState() const;

    /**
     * @brief Setter for the tooltip of the node. Will be displayed when
     * hovering over the node.
     * @param tooltip New Tooltip
     */
    void setToolTip(QString const& tooltip);

    /**
     * @brief Tooltip of the node
     * @return Tooltip
     */
    QString const& toolTip() const noexcept;
    [[deprecated("use `toolTip` instead")]]
    QString const& tooltip() const noexcept { return toolTip(); }

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
     * @brief Will create a unique object name based on the node caption
     */
    void updateObjectName();

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
    QString const& modelName() const noexcept;

    /**
     * @brief Returns a list of the input or output ports depending on the port
     * type
     * @param type Port type (input or output)
     * @return Ports
     */
    std::vector<NodePort> const& ports(PortType type) const noexcept(false);

    /**
     * @brief Returns a view of the input or output ports respectively. Node
     * ports are not mutable. Removing or adding an port invalidates the
     * returned object.
     * @throws Throws if the port type is invalid.
     * @param type Port type (input or output)
     * @return Non mutable view on the ports
     */
    View<NodePort> getPorts(PortType type) const noexcept(false);

    /**
     * @brief Returns a span of the input or output ports respectively. Node
     * ports are mutable. Removing or adding an port invalidates the
     * returned object.
     * @throws Throws if the port type is invalid.
     * @param type Port type (input or output)
     * @return Mutable view on the ports
     */
    Span<NodePort> getPorts(PortType type) noexcept(false);

    /**
     * @brief Returns the port for the port id
     * @param id Port id
     * @return Port. May be null
     */
    NodePort* port(PortId id) noexcept;
    NodePort const* port(PortId id) const noexcept;

    /**
     * @brief Returns the port index of the port id and its port type.
     * @param type Port type (input or output)
     * @param id Port id
     * @return Port index. May be invalid.
     */
    PortIndex portIndex(PortType type, PortId id) const noexcept(false);

    /**
     * @brief Returns the port index and port type of the port id.
     * @param type Port type (input or output)
     * @param id Port id
     * @return Port index and port type. May be invalid.
     */
    PortLocation portIndex(PortId id) const noexcept(false);

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
     * @return Port id. May be invalid.
     */
    PortId portId(PortType type, PortIndex idx) const noexcept(false);

    /**
     * @brief Returns whether a port is connected
     * @param id Port id to check
     * @return Is port connected
     */
    [[deprecated("Use `NodePort::isConnected()` instead")]]
    bool isPortConnected(PortId portId) const;

    Q_INVOKABLE GraphUserVariables const* userVariables() const;

signals:

    /**
     * @brief Notifies listeners that the output data of a node changed.
     * May trigger a node execution, if auto evaluation is enabled. Should be
     * triggered by the "user" if the node has changed internal data and
     * a reevaluation is necessary to update the output data.
     */
    void invalidateNode();
    [[deprecated("use `invalidateNode` instead")]]
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
    void inputDataUpdated(PortId portId = invalid<PortId>());
    [[deprecated("use `inputDataUpdated` instead")]]
    void inputDataRecieved(PortId portId = invalid<PortId>());

    /**
     * @brief Emitted if node specific data has changed (caption, number of
     * ports etc.). May be invoked by the "user" to update the graphical node
     * representation in case a port has changed for example.
     */
    void nodeChanged();

    /**
     * @brief Emitted when the node has changed its position. Triggered by
     * `setPos`.
     */
    void nodePositionChanged(QPrivateSignal);

    /**
     * @brief Emitted when the node has changed its size. Triggered by
     * `setSize`.
     */
    void nodeSizeChanged(QPrivateSignal);

    /**
     * @brief Emitted just before the node is deleted. Similar to
     * `QObject::destroyed` with the difference that one may still access
     * Node members (not members of derived classes!).
     */
    void nodeAboutToBeDeleted(NodeId nodeId, QPrivateSignal);

    /**
     * @brief Emitted if port specific data has changed (e.g. port caption).
     * May be invoked by the "user" to update the graphical node representation
     * @param id id of port that changed
     */
    void portChanged(PortId id);

    /**
     * @brief Will be called internally before deleting a port.
     * @param type Port type (input or output)
     * @param idx Affected index
     */
    void portAboutToBeDeleted(PortType type, PortIndex idx, QPrivateSignal);

    /**
     * @brief Will be emitted just after a port was deleted
     * @param type Port type (input or output)
     * @param idx Old index
     */
    void portDeleted(PortType type, PortIndex idx, QPrivateSignal);

    /**
     * @brief Will be called internally before inserting a port.
     * @param type Port type (input or output)
     * @param idx Affected index
     */
    void portAboutToBeInserted(PortType type, PortIndex idx, QPrivateSignal);

     /**
     * @brief Will be emitted just after a port was inserted
     * @param type Port type (input or output)
     * @param idx New index
     */
    void portInserted(PortType type, PortIndex idx, QPrivateSignal);

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

    /**
     * @brief Emitted once the `isActive` flag changes.
     */
    void isActiveChanged(QPrivateSignal);

    /**
     * @brief Emitted once the node evaluation state changes
     */
    void nodeEvalStateChanged();

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
     * @brief This function invoked when certain events are triggered allowing
     * the node to react accordingly.
     * @param event Event
     */
    virtual void nodeEvent(NodeEvent const* event);

    /**
     * @brief Main evaluation method to override. Will be called once, the
     * "user" has to calculate and set the data for all output ports. Will not
     * be called if any required input port has no valid data associated
     * (see PortPolicy)
     */
    virtual void eval();

    /**
     * @brief Can be called to indicate that the node evaluation failed.
     */
    void evalFailed();

    /**
     * @brief Should be called within the constructor. Used to register
     * the widget factory, used for creating the embedded widget within the
     * intelli graphs.
     * @param factory Widget factory
     */
    [[deprecated("Reimplement `NodeUI::centralWidgetFactory` instead.")]]
    void registerWidgetFactory(WidgetFactory factory);
    [[deprecated("Reimplement `NodeUI::centralWidgetFactory` instead.")]]
    void registerWidgetFactory(WidgetFactoryNoArgs factory);

    /**
     * @brief Sets a node flag
     * @param flag Flag to set
     * @param enable Whether to enable or disable the flag
     */
    void setNodeFlag(NodeFlag flag, bool enable = true);

    [[deprecated("Use `Node::setNodeFlags` or `Node::setNodeFlag` instead.")]]
    void setNodeFlag(NodeFlags flags, bool enable = true)
    {
        setNodeFlag(static_cast<NodeFlag>(flags.value()), enable);
    }

    /**
     * @brief Overload, that accepts a custom node flag
     * @param flag Flag(s) to set
     * @param enable Whether to enable or disable the flag(s)
     */
    void setNodeFlags(NodeFlags flags);

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
    PortId addInPort(NodePort port, PortPolicy policy = DefaultPortPolicy) noexcept(false);

    /**
     * @brief Appends the output port
     * @param port Port data to append
     * @return Port id
     */
    PortId addOutPort(NodePort port) noexcept(false);

    /**
     * @brief Inserts an input port at the given location
     * (-1 will append to back)
     * @param port Port data to insert
     * @param idx Where to insert the port
     * @param policy Input port policy
     * @return Port id
     */
    PortId insertInPort(NodePort port, int idx, PortPolicy policy = DefaultPortPolicy) noexcept(false);

    /**
     * @brief Inserts an output port at the given location
     * (-1 will append to back)
     * @param port Port data to insert
     * @param idx Where to insert the port
     * @return Port id
     */
    PortId insertOutPort(NodePort port, int idx) noexcept(false);

    /**
     * @brief Helper method for inserting a port. Prefer the explicit insert/add
     * methods.
     * @param type Whether to insert an in or out port
     * @param port Port data to insert
     * @param idx Where to insert the port
     * @return Port id
    */
    PortId insertPort(PortType type, NodePort port, int idx = -1) noexcept(false);

    /**
     * @brief Removes the port specified by id
     * @param id Port to remove
     * @return Success
     */
    bool removePort(PortId id);

    /**
     * @brief Returns the node data of the specified port. No conversion is
     * performed.
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
     * desired type. Cannot make use of conversions.
     * @param id Port id (output or input)
     * @deprecated Remove pointer from template type to make use of conversions
     * @return Port data
     */
    template <typename T,
              typename U = std::remove_pointer_t<T>,
              typename = std::enable_if_t<std::is_pointer<T>::value>>
    [[deprecated("remove pointer from template type: `nodeData<T>`")]]
    U const* nodeData(PortId id) const
    {
        return qobject_cast<U const*>(nodeData(id).get());
    }

    /**
     * @brief Overload that casts the node data of the specified port to the
     * desired type. Performs a conversion if necessary.
     * @param id Port id (output or input)
     * @return Port data
     */
    template <typename T,
              typename = std::enable_if_t<!std::is_pointer<T>::value>>
    Ptr<unwrap_type_t<T>> nodeData(PortId id) const
    {
        return convert<T>(nodeData(id));
    }

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;

    // hide object name setter
    using GtObject::setObjectName;
};

/**
 * @brief Helper struct that yields the desired identification of a node
 * depending on the template parameter.
 *
 * Usage:
 * - NodeId id = get_node_id<NodeId>{}(ptr);
 * - NodeUuid uuid = get_node_id<NodeUuid>{}(ptr);
 */
template <typename NodeId_t>
struct get_node_id;

template <>
struct get_node_id<NodeId>
{
    template <typename T>
    NodeId operator()(T const* o) { assert(o); return o->nodeId(); }
    NodeId operator()(Node const* node) { assert(node); return node->id(); }
    NodeId operator()(NodeId nodeId) { return nodeId; }
};

template <>
struct get_node_id<NodeUuid>
{
    template <typename T>
    NodeId operator()(T const* o) { assert(o); return o->nodeUuid(); }
    NodeUuid operator()(Node const* node) { assert(node); return node->uuid(); }
    NodeUuid operator()(NodeUuid nodeUuid) { return nodeUuid; }
};

/**
 * @brief Returns the relative path to the root node.
 * @param node Target node
 * @return Relative node path
 */
inline QString
relativeNodePath(Node const& node)
{
    auto const* root = node.template findRoot<Node const*>();
    if (!root) return node.caption();

    return root->caption() + (node.objectPath().remove(root->objectPath())).replace(';', '/');
}

inline QString
relativeNodePath(Node const* node)
{
    return node ? relativeNodePath(*node) : QStringLiteral("NULL");
}

} // namespace intelli


#endif // GT_INTELLI_NODE_H
