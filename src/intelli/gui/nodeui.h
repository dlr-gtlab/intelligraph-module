/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NODEUI_H
#define GT_INTELLI_NODEUI_H

#include <intelli/gui/portuiaction.h>
#include <intelli/exports.h>
#include <intelli/flags.h>

#include <gt_objectui.h>

#include <functional>

class QGraphicsWidget;

namespace intelli
{

class Node;
class NodeUIData;
class NodeGeometry;
class NodePainter;
class NodeGraphicsObject;
class Graph;
class DynamicNode;

class GT_INTELLI_EXPORT NodeUI : public GtObjectUI
{
    Q_OBJECT

public:

    /// pointer type for widget factory
    using QGraphicsWidgetPtr = std::unique_ptr<QGraphicsWidget>;

    /// central widget factory, see `NodeUI::centralWidgetFactory` for more
    /// details
    using WidgetFactoryFunction =
        std::function<std::unique_ptr<QGraphicsWidget> (Node& source, NodeGraphicsObject& object)>;

    /// forward action method for port action
    using PortActionFunction = typename PortUIAction::ActionMethod;

    /// custom deleter signature
    using CustomDeleteFunctor = std::function<bool (Node*)>;
    /// function signature to check if deleter is applicable
    using EnableCustomDeleteFunctor = std::function<bool (Node const*)>;

    /// Option enum, can be used to deactivate certain default actions
    enum Option : unsigned
    {
        NoOption = 0,
        /// Stops the object to populate the default node actions. Can use
        /// `defaultNodeActions` and `initializeNodeActions` to customize the
        /// order and apperance of node actions.
        CustomNodeActionsOrder = 1 << 0,
        /// Stops the object to populate the default port actions. Can use
        /// `defaultPortActions` and `initializePortActions` to customize the
        /// order and apperance of port actions.
        CustomPortActionsOrder = 1 << 1,
        CustomOrder = CustomNodeActionsOrder | CustomPortActionsOrder,

        UserOption = 1 << 10,

        NoDefaultNodeActions = CustomNodeActionsOrder,
        NoDefaultPortActions = CustomPortActionsOrder,
        NoDefaultActions     = CustomOrder,
    };

    using Options = UFlags<Option>;

    Q_INVOKABLE NodeUI(Options options = NoOption);
    NodeUI(NodeUI const&) = delete;
    NodeUI(NodeUI&&) = delete;
    NodeUI& operator=(NodeUI const&) = delete;
    NodeUI& operator=(NodeUI&&) = delete;
    ~NodeUI() override;
    
    /**
     * @brief Returns a painter object, used to paint the graphics object
     * given the node geomtry. Can be used to override the default
     * implementation.
     * @param object Graphics object on which the painter should operate
     * @param geometry Node geometry which defines the position and size of
     * ports, the caption etc.
     * @return Node painter object
     */
    virtual std::unique_ptr<NodePainter> painter(NodeGraphicsObject const& object,
                                                 NodeGeometry const& geometry) const;

    /**
     * @brief Returns a geomtry object, used to tell graphics object where
     * ports, the caption etc. are placed. Can be used to override the default
     * implementation.
     * @param node Node to operate on
     * @return Node geometry object
     */
    virtual std::unique_ptr<NodeGeometry> geometry(NodeGraphicsObject const& object) const;

    /**
     * @brief Returns an ui-data object. This object hold various properties
     * and is used to customize behavior and rendering.
     * @param node Node
     * @return node ui data
     */
    std::unique_ptr<NodeUIData> uiData(Node const& node) const;

    CustomDeleteFunctor customDeleteAction(Node const& node) const;

    /**
     * @brief Icon for the object (in the explorer)
     * @param obj Object
     * @return icon
     */
    QIcon icon(GtObject* obj) const override;

    /**
     * @brief Icon to display in the header of the node (in the graph view)
     * @param node Node
     * @return icon
     */
    virtual QIcon displayIcon(Node const& node) const;

    /**
     * @brief Returns the widget factory for the given node object. The factory
     * recieves instances of `Node`and `NodeGraphicsObject` as arguments and
     * should return a `QGraphicsWidget`. If a `QWidget` is needed wrap it using
     * `NodeUI::convertToGraphicsWidget`.
     *
     * Regarding the arguments of the factory:
     *  - `source` is the node that the widget is designed for. As such, it
     *  should be used for setting up the widget.
     *  - `object` is the graphics object the widget will be embedded into.
     *  The object is alo associated with a node, but this node may be different
     *  from `source`. This might be case if the widget is embedded into another
     *  graphics object of a node. The graphics object should many be used for
     *  accesing the painter or similar settings.
     * @param node Node object, that determines which factory should be
     * registered. Should not be used within the factory.
     * @return Widget factory
     */
    virtual WidgetFactoryFunction centralWidgetFactory(Node const& node) const;

    /**
     * @brief Converts a QWidget into a QGraphicsWidget. Updates the palette
     * to match the node's style.
     * @param widget Widget to convert
     * @param object Graphics object the widget will be embedded into
     * @return
     */
    GT_NO_DISCARD
    static QGraphicsWidgetPtr convertToGraphicsWidget(std::unique_ptr<QWidget> widget,
                                                      NodeGraphicsObject& object);

    /**
     * @brief Returns the list of mdi items to open the object with
     * @param obj Object to open
     * @return Class names of compatible mdi items
     */
    QStringList openWith(GtObject* obj) override;

    /**
     * @brief Returns the list of all port actions registered
     * @return
     */
    QList<PortUIAction> const& portActions() const;

    /**
     * @brief Opens the Edit-User-Variables-Dialog for the root graph `obj`.
     * @param obj Object must be root graph.
     */
    static void editUserVariables(GtObject* obj);

protected:

    /// enum contaning the default node actions. May be used in conjunction
    /// with `defaultNodeActions` to customize the order of node actions.
    enum NodeAction : unsigned
    {
        ExecuteNodeAction = 1 << 1,
        SetActiveNodeAction = 1 << 2,
        RenameNodeAction = 1 << 3,
        CustomNodeAction = 1 << 4,
        AddPortNodeAction = 1 << 5,
        OtherNodeAction = 1 << 6,

        UserNodeAction = 1 << 8
    };

    /// enum contaning the default port actions. May be used in conjunction
    /// with `defaultNodeActions` to customize the order of port actions.
    enum PortAction : unsigned
    {
        EditPortAction = 1 << 1,
        DeletePortAction = 1 << 2,
        CustomPortAction = 1 << 3,
        OtherPortAction = 1 << 4,

        UserPortAction = 1 << 8
    };

    template<typename Enum, typename ActionType>
    class ActionList;

    using NodeActionList = ActionList<NodeAction, GtObjectUIAction>;
    using PortActionList = ActionList<PortAction, PortUIAction>;

    virtual NodeActionList defaultNodeActions() const;

    virtual PortActionList defaultPortActions() const;

    void initializeNodeActions(NodeActionList const& actions);

    void initializePortActions(PortActionList const& actions);


    /** HELPERS FOR VERFIY AND VISBILITY FOR NODE ACTIONS **/

    /**
     * @brief Casts the object to a node object. Can be used for validation
     * @param obj Object to cast
     * @return node object (may be null)
     */
    static Node* toNode(GtObject* obj);
    static Node const* toConstNode(GtObject const* obj);

    /**
     * @brief Casts the object to an intelligraph object. Can be used for
     * validation
     * @param obj Object to check
     * @return intelligraph object (may be null)
     */
    static Graph* toGraph(GtObject* obj);
    static Graph const* toConstGraph(GtObject const* obj);

    /**
     * @brief Casts the object to a dynamic node object. Can be used for
     * validation
     * @param obj Object to cast
     * @return node object (may be null)
     */
    static DynamicNode* toDynamicNode(GtObject* obj);
    static DynamicNode const* toConstDynamicNode(GtObject const* obj);

    /**
     * @brief Returns whether this object is a root graph
     * @param obj Object to check
     * @return is object a root graph
     */
    static bool isRootGraph(GtObject const* obj);

    /** NODE ACTIONS **/

    static GtObjectUIAction makeSeparator();

    static GtObjectUIAction makeSingleAction(QString const& text, ActionFunction f);

    /**
     * @brief Prompts the user to rename the node
     * @param obj Object to rename
     */
    static void renameNode(GtObject* obj);

    /**
     * @brief Triggers the execution of a node
     * @param obj
     */
    static void executeNode(GtObject* obj);

    /** PORT ACTIONS **/

    /**
     * @brief Adds a port action and returns a reference to the added action,
     * which can be used to customize the action. Reference may become invalid
     * if another port action is added.
     * @param actionText Text of action
     * @param actionMethod Method to execute
     * @return Reference to port action
     */
    PortUIAction& addPortAction(QString const& actionText,
                                PortActionFunction actionMethod);

    static PortUIAction makePortAction(QString const& actionText,
                                       PortActionFunction actionMethod);

    static PortUIAction makePortSeparator();

    /**
     * @brief Prompts the user and adds an input port to the given dynamic node
     * @param obj
     */
    static void addDynamicInPort(GtObject* obj);

    /**
     * @brief Prompts the user and adds an output port to the given dynamic node
     * @param obj
     */
    static void addDynamicOutPort(GtObject* obj);

    /**
     * @brief Prompts the user to edit the given dynamic port
     * @param obj
     * @param type
     * @param idx
     */
    static void editDynamicPort(Node* obj, PortType type, PortIndex idx);

    /**
     * @brief Deletes a dynamic port
     * @param obj
     * @param type
     * @param idx
     */
    static void deleteDynamicPort(Node* obj, PortType type, PortIndex idx);

    /** HELPERS FOR VERFIY AND VISBILITY ON PORT ACTIONS **/

    static bool isInputPort(Node* obj, PortType type, PortIndex idx);

    static bool isOutputPort(Node* obj, PortType type, PortIndex idx);

    /**
     * @brief Similar to `toDynamicNode`. Can be used for validating port
     * actions
     * @param obj Object to cast
     * @return node object (may be null)
     */
    static bool isDynamicPort(Node* obj, PortType type, PortIndex idx);

    [[deprecated("use `toDynamicNode` instead")]]
    static bool isDynamicNode(Node* obj, PortType type, PortIndex idx);

    /** DDELETERS **/

    /**
     * @brief Allows to register a dedicated delete action that will be called
     * when invoked by the user (i.e. in a GraphicsScene instance).
     * @param actionText Name of the delete action
     * @param deleteFunctor Deletion action
     * @param isDeletable Functor that yields whether the delete action is
     * enabled.
     */
    void addCustomDeleteAction(QString const& actionText,
                               CustomDeleteFunctor deleteFunctor,
                               EnableCustomDeleteFunctor isDeletable);

    /**
     * @brief Overload, that uses a default name for the delete action.
     * @param deleteFunctor Deletion action
     * @param isDeletable Functor that yields whether the delete action is
     * enabled.
     */
    void addCustomDeleteAction(CustomDeleteFunctor deleteFunctor,
                               EnableCustomDeleteFunctor isDeletable);

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;

    static bool deleteDummyNode(Node* node);

    /**
     * @brief Checks if node can be renamed (i.e. node should be valid but not unique)
     * @param obj Object to check
     * @return is object renamable
     */
    static bool canRenameNodeObject(GtObject* obj);

    /**
     * @brief helper method for setting the active flag of a node
     * @param obj
     */
    template <bool State>
    static void setActive(GtObject* obj) { setActive(obj, State); }

    /**
     * @brief sets the active flag of a node
     * @param obj Node to set the flag of
     * @param state New active state
     */
    static void setActive(GtObject* obj, bool state);
};

} // namespace intelli

/**
 * @brief Helper class to customize the order of node and port actions.
 * Must use `makeSingleAction` or `makePortAction` to avoid adding methods
 * twice.
 */
template<typename Enum, typename ActionType>
class intelli::NodeUI::ActionList
{
    using EnumType = std::underlying_type_t<Enum>;
    struct Entry
    {
        ActionType action;
        EnumType value;
    };

    std::vector<Entry> actions;
    EnumType nextEnum{0};

    inline auto findByEnum(EnumType value)
    {
        return [value](Entry const& entry){ return entry.value == value; };
    }

    inline static bool isSeparator(Entry const& entry)
    {
        return entry.action.text().isEmpty();
    }

    inline EnumType getNext(EnumType value, EnumType fallback)
    {
        return (value == 0) ? fallback : value;
    }

public:

    using iterator = typename decltype(actions)::iterator;

    /* ITERATORS */
    auto begin() { return actions.begin(); }
    auto begin() const { return actions.begin(); }
    auto cbegin() const { return actions.cbegin(); }
    auto end() { return actions.end(); }
    auto end() const { return actions.end(); }
    auto cend() const { return actions.cend(); }

    void reserve(size_t size) { actions.reserve(size); }

    void remove(EnumType value)
    {
        actions.erase(std::remove_if(actions.begin(), actions.end(), findByEnum(value)),
                      actions.end());
    }

    /// finds the iterator before the first action of type `value` and before any separator
    iterator before(EnumType value)
    {
        return std::find_if(actions.begin(), actions.end(), findByEnum(value));
    }

    /// finds the iterator before the first action of type `value` and after any separator
    iterator beforeSeparator(EnumType value)
    {
        auto iter = before(value);
        if (iter != actions.end() && iter != actions.begin())
        {
            while (true)
            {
                auto before = std::prev(iter);
                if (!isSeparator(*before)) break;
                iter = before;
            }
        }
        return iter;
    }

    /// inserts `action` before the first action of type `value` and after any separator
    iterator insertBefore(EnumType value, ActionType action, EnumType next = 0)
    {
        return insert(before(value), std::move(action), getNext(next, value));
    }

    /// inserts `action` before the first action of type `value` and before any separator
    iterator insertBeforeSeparator(EnumType value, ActionType action, EnumType next = 0)
    {
        return insert(beforeSeparator(value), std::move(action), getNext(next, value));
    }

    /// finds the iterator after the last action of type `value` and before any separator
    iterator after(EnumType value)
    {
        auto riter = std::find_if(actions.rbegin(), actions.rend(), findByEnum(value));
        if (riter == actions.rend()) return actions.end();
        return riter.base();
    }

    /// finds the iterator after the last action of type `value` and after any separator
    iterator afterSeparator(EnumType value)
    {
        auto iter = after(value);
        while (iter != actions.end() && iter != actions.begin() && isSeparator(*iter))
        {
            iter = std::next(iter);
        }
        return iter;
    }

    /// inserts `action` after the last action of type `value` and before any separator
    iterator insertAfter(EnumType value, ActionType action, EnumType next = 0)
    {
        return insert(after(value), std::move(action), getNext(next, value));
    }

    /// inserts `action` after the last action of type `value` and after any separator
    iterator insertAfterSeparator(EnumType value, ActionType action, EnumType next = 0)
    {
        return insert(afterSeparator(value), std::move(action), getNext(next, value));
    }

    /// inserts `action` before the given iterator
    iterator insert(iterator iter, ActionType action, EnumType next = 0)
    {
        if (nextEnum > 0)
        {
            next = nextEnum;
            nextEnum = 0;
        }
        return actions.insert(iter, Entry{std::move(action), next});
    }

    /// appends `action` last
    ActionList& operator<<(ActionType action) { append(std::move(action)); return *this; }
    /// sets the enum value for the next added action under which the action
    /// may be found
    ActionList& operator<<(EnumType next) { nextEnum = next; return *this; }

    /// appends the action last
    iterator append(ActionType action, EnumType value = 0)
    {
        return insert(actions.end(), std::move(action), value);
    }
};

#endif // GT_INTELLI_NODEUI_H
