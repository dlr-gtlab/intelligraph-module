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

    class ActionChainOperator;
    class PortActionChainOperator;

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

    /// enum contaning the order priorities for the default node actions.
    struct OrderPriority
    {
        enum Value : int
        {
            EvaluationAction = 0,
            CustomAction = 5,
            PortAction = 10,

            BeforeEvaluationActions = EvaluationAction - 1,
            AfterEvaluationActions = EvaluationAction + 1,

            BeforeCustomActions = CustomAction - 1,
            AfterCustomActions = CustomAction + 1,

            BeforePortActions = PortAction - 1,
            AfterPortActions = PortAction + 1,
        };
    };

    /// Option enum, can be used to deactivate certain default actions
    enum Option : unsigned
    {
        NoOption = 0,
        NoDefaultNodeActions = 1 << 0,
        NoDefaultPortActions = 1 << 1,
        NoDefaultActions     = NoDefaultNodeActions | NoDefaultPortActions,
        NoRenameOption       = 1 << 2,
        NoDynamicPortActions = 1 << 3,
        UserOption = 1 << 10,
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

    /**
     * @brief Returns the custom delete action for `node`
     * @param node Node
     * @return Delete function for `node`
     */
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

    /// chain operator to check if object is a node
    static ActionChainOperator const isNode;
    /// chain operator to check if object is a graph
    static ActionChainOperator const isGraph;
    /// chain operator to check if object is a root graph
    static ActionChainOperator const isRootGraph;
    /// chain operator to check if object is a dynamic node
    static ActionChainOperator const isDynamicNode;
    /// chain operator to check if node is active
    static ActionChainOperator const isNodeActive;

    /** NODE ACTIONS **/

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

    /**
     * @brief Adds a separator for the port context menu
     */
    void addPortSeparator();

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

    static PortActionChainOperator const isInputPort;
    static PortActionChainOperator const isOutputPort;
    static PortActionChainOperator const isDynamicPort;

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

class NodeUI::ActionChainOperator
{
    using NodeActionVerifyMethod = std::function<bool(GtObject*)>;

    NodeActionVerifyMethod f;

public:

    ActionChainOperator(NodeActionVerifyMethod m) : f(std::move(m)) {}

    inline NodeActionVerifyMethod& get() & { return f; }
    inline NodeActionVerifyMethod&& get() && { return std::move(f); }
    inline NodeActionVerifyMethod const& get() const& { return f; }

    /// AND operator
    template <typename Functor>
    inline ActionChainOperator operator&&(Functor fOther) const
    {
        return {
            [a = f, b = std::move(fOther)](GtObject* obj){
                return a(obj) && b(obj);
            }
        };
    }
    /// OR operator
    template <typename Functor>
    inline ActionChainOperator operator||(Functor fOther) const
    {
        return {
            [a = f, b = std::move(fOther)](GtObject* obj){
                return a(obj) || b(obj);
            }
        };
    }
    /// NOT operator
    inline ActionChainOperator operator!() const
    {
        return {
            [a = f](GtObject* obj){
                return !a(obj);
            }
        };
    }
    /// call operator
    inline bool operator()(GtObject* obj) const { return f(obj); }
    /// cast operator
    inline operator NodeActionVerifyMethod() const& { return f; }
    inline operator NodeActionVerifyMethod&&() && { return std::move(f); }
};

class NodeUI::PortActionChainOperator
{
    using PortActionVerifyMethod = std::function<bool(Node*, PortType, PortIndex)>;

    PortActionVerifyMethod f;

public:

    PortActionChainOperator(PortActionVerifyMethod m) : f(std::move(m)) {}

    inline PortActionVerifyMethod& get() & { return f; }
    inline PortActionVerifyMethod&& get() && { return std::move(f); }
    inline PortActionVerifyMethod const& get() const& { return f; }

    /// AND operator
    template <typename Functor>
    inline PortActionChainOperator operator&&(Functor fOther) const
    {
        return {
                [a = f, b = std::move(fOther)](Node* obj, PortType type, PortIndex idx){
                return a(obj, type, idx) && b(obj, type, idx);
            }
        };
    }
    /// OR operator
    template <typename Functor>
    inline PortActionChainOperator operator||(Functor fOther) const
    {
        return {
                [a = f, b = std::move(fOther)](Node* obj, PortType type, PortIndex idx){
                return a(obj, type, idx) || b(obj, type, idx);
            }
        };
    }
    /// NOT operator
    inline PortActionChainOperator operator!() const
    {
        return {
                [a = f](Node* obj, PortType type, PortIndex idx){
                return !a(obj, type, idx);
            }
        };
    }
    /// call operator
    inline bool operator()(Node* obj, PortType type, PortIndex idx) const
    {
        return f(obj, type, idx);
    }

    /// cast operator
    inline operator PortActionVerifyMethod() const& { return f; }
    inline operator PortActionVerifyMethod&&() && { return std::move(f); }
};

} // namespace intelli

#endif // GT_INTELLI_NODEUI_H
