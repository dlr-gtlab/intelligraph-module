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

#include <intelli/globals.h>
#include <intelli/exports.h>

#include <gt_typetraits.h>
#include <gt_object.h>

#include <QWidget>

namespace intelli
{

enum NodeFlag : size_t
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
    /// default node flags
    DefaultNodeFlags = NoFlag,

    /// base flag for user defined node flags
    UserFlag = 1 << 16
};

using NodeFlags = size_t;

/// mask to check if node is resizable
constexpr size_t IsResizableMask =
    Resizable | ResizableHOnly;

/// mask to check if node should be evaluated in separate thread
constexpr size_t IsDetachedMask = 1 << 0;
/// mask to check if node should be evaluated in main thread
constexpr size_t IsBlockingMask = 1 << 1;
/// mask to check if node should be evaluated exclusively
constexpr size_t IsExclusiveMask = 1 << 2;

enum class NodeEvalMode : size_t
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
    /// Inidcates that the inputs of the node should be forwarded to the outputs
    /// of the node
    ForwardInputsToOutputs = 1 << 3 | IsBlockingMask,
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

namespace exec
{

GT_INTELLI_EXPORT bool blockingEvaluation(Node& node, NodeDataInterface& model);

GT_INTELLI_EXPORT bool detachedEvaluation(Node& node, NodeDataInterface& model);

GT_INTELLI_EXPORT void setNodeDataInterface(Node& node, NodeDataInterface* model);

GT_INTELLI_EXPORT NodeDataInterface* nodeDataInterface(Node& node);

/**
 * @brief Triggers the evaluation of the node.
 * @return success
 */
GT_INTELLI_EXPORT bool triggerNodeEvaluation(Node& node, NodeDataInterface& model);

}

/**
 * @brief Attempts to convert `data` to into the desired type. If
 * no conversion exists or the conversion fails, a nullptr is returned.
 * @param data Data to convert
 * @param to Target type Id to convert the data into
 * @return Converted data (may be null)
 */
GT_INTELLI_EXPORT NodeDataPtr convert(NodeDataPtr const& data, TypeId const& to);

/**
 * @brief Convenience function that performs a conversion of `data` into the
 * desired type `T`. If
 * no conversion exists or the conversion fails, a nullptr is returned.
 * @param data Data to convert
 * @return Converted data of type `T` (may be null)
 */
template <typename T>
std::shared_ptr<T const> convert(NodeDataPtr data)
{
    return std::static_pointer_cast<T const>(
        convert(data, T::staticMetaObject.className())
    );
}

/**
 * @brief Creates a base widget that has a simple layout attached. Can be used
 * for widgets, that have trouble resizing correctly.
 * @return Widget pointer (never null)
 */
GT_INTELLI_EXPORT std::unique_ptr<QWidget> makeBaseWidget();

class GT_INTELLI_EXPORT Node : public GtObject
{
    Q_OBJECT
    
    friend class INode;
    friend class NodeGraphicsObject;

public:

    using NodeId        = intelli::NodeId;
    using NodeFlag      = intelli::NodeFlag;
    using NodeFlags     = intelli::NodeFlags;
    using NodeEvalMode  = intelli::NodeEvalMode;
    using NodeEvalState = intelli::NodeEvalState;
    using PortType      = intelli::PortType;
    using PortId        = intelli::PortId;
    using PortIndex     = intelli::PortIndex;
    using Position      = intelli::Position;
    using NodeDataPtr   = intelli::NodeDataPtr;

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

    /// Port info struct
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

        PortInfo(PortInfo const& other) = default;
        PortInfo(PortInfo&& other) = default;
        PortInfo& operator=(PortInfo const& other) = delete;
        PortInfo& operator=(PortInfo&& other) = default;
        ~PortInfo() = default;

        /// creates a PortInfo struct with a custom port id
        template<typename ...T>
        static PortInfo customId(PortId newPortId, T&&... args)
        {
            PortInfo pd(std::forward<T>(args)...);
            pd.m_id = newPortId;
            return pd;
        }

        /// Performs a copy assignment using `other` but keeps old id
        void assign(PortInfo other)
        {
            PortInfo tmp(std::move(other));
            tmp.m_id = m_id;
            swap(tmp);
        }

        /// creates a copy of this object but resets the id parameter
        PortInfo copy() const
        {
            PortInfo pd(*this);
            pd.m_id = invalid<PortId>();
            return pd;
        }

        /// swap
        void swap(PortInfo& other) noexcept
        {
            using std::swap;
            swap(typeId, other.typeId);
            swap(caption, other.caption);
            swap(captionVisible, other.captionVisible);
            swap(visible, other.visible);
            swap(optional, other.optional);
            swap(m_isConnected, other.m_isConnected);
            swap(m_id, other.m_id);
        }

        // setters to allow function chaining
        PortInfo& setCaption(QString v) { caption = std::move(v); return *this; }
        PortInfo& setToolTip(QString v) { toolTip = std::move(v); return *this; }
        PortInfo& setCaptionVisible(bool v) { captionVisible = v; return *this; }
        PortInfo& setVisible(bool v) { visible = v; return *this; }
        PortInfo& setOptional(bool v) { optional = v; return *this; }

        /// type id for port data (classname)
        TypeId typeId;
        /// custom port caption (optional)
        QString caption;
        /// custom tooltip
        QString toolTip;
        /// whether port caption should be visible
        bool captionVisible = true;
        /// whether the port is visible at all
        bool visible = true;
        /// whether the port is required for the node evaluation
        bool optional = true;

        /**
         * @brief Returns the port id
         * @return port id
         */
        inline PortId id() const { return m_id; }

        /**
         * @brief Whether port is connected
         * @return Whether port is connected
         */
        inline bool isConnected() const { return m_isConnected; }

    private:
        /// whether port is connected
        bool m_isConnected{false};
        /// read only PortId
        PortId m_id{};
        
        friend class Node;
    };
    using PortData [[deprecated("use PortInfo")]] = PortInfo;
    
    /**
     * @brief Helper method to create a `PortInfo` struct given a `TypeId`.
     * @param typeId TypeId
     * @return `PortInfo` struct
     */
    static PortInfo makePort(TypeId typeId) { return PortInfo{std::move(typeId)}; }

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
    QString const& tooltip() const;

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
    [[deprecated("Use `PortInfo::isConnected()` instead")]]
    bool isPortConnected(PortId portId) const;

signals:

    /**
     * @brief Triggers the evaluation of node. It is not guranteed to be
     * evaluated, as the underling graph execution model must be active.
     * Should be triggered if the node has changed internal data and requires
     * reevaluation.
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
     * flag `Evaluating` automatically.
     */
    [[deprecated]]
    void computingStarted();
    
    /**
     * @brief Emitted once the node evaluation has finished. Will update the node
     * flag `Evaluating` automatically.
     */
    [[deprecated("use `evaluated` signal instead")]]
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

    /**
     * @brief Emitted once the `isActive` flag changes.
     */
    void isActiveChanged();

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
    void registerWidgetFactory(WidgetFactory factory);
    void registerWidgetFactory(WidgetFactoryNoArgs factory);

    /**
     * @brief Sets a node flag
     * @param flag Flag to set
     * @param enable Whether to enable or disable the flag
     */
    void setNodeFlag(NodeFlag flag, bool enable = true);

    /**
     * @brief Overload, that accepts a custom node flag
     * @param flag Flag(s) to set
     * @param enable Whether to enable or disable the flag(s)
     */
    inline void setNodeFlag(size_t flag, bool enable = true)
    {
        return setNodeFlag((NodeFlag)flag, enable);
    }

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
              std::enable_if_t<std::is_pointer<T>::value, bool> = true>
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
              std::enable_if_t<!std::is_pointer<T>::value, bool> = true>
    std::shared_ptr<T const> nodeData(PortId id) const
    {
        return convert<T>(nodeData(id));
    }

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;

    // hide object name setter
    using GtObject::setObjectName;
};

inline void swap(Node::PortInfo& a, Node::PortInfo& b) noexcept { a.swap(b); }

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
    NodeId operator()(Node const* node) { assert(node); return node->id(); }
};

template <>
struct get_node_id<NodeUuid>
{
    NodeUuid operator()(Node const* node) { assert(node); return node->uuid(); }
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
