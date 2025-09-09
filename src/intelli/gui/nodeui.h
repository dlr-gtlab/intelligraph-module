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

#include <gt_objectui.h>
#include <thirdparty/tl/optional.hpp>

#include <functional>

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

    using CustomDeleteFunctor = std::function<bool (Node*)>;
    using EnableCustomDeleteFunctor = std::function<bool (Node const*)>;

    /// Option enum, can be used to deactive certain default actions
    enum Option
    {
        NoOption = 0,
        /// Deactivates all default actions
        NoDefaultActions,
        /// Deactivates the default port actions for dynamic nodes
        NoDefaultPortActions,
    };

    using PortActionFunction = typename PortUIAction::ActionMethod;

    Q_INVOKABLE NodeUI(Option option = NoOption);
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
     * @brief Returns the list of mdi items to open the object with
     * @param obj Object to open
     * @return Class names of compatible mdi items
     */
    QStringList openWith(GtObject* obj) override;

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

    static bool isRootGraph(GtObject const* obj);

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

    /**
     * @brief Adds an input port to a dynamic node
     * @param obj
     */
    static void addInPort(GtObject* obj);

    /**
     * @brief Adds an output port to a dynamic node
     * @param obj
     */
    static void addOutPort(GtObject* obj);

    /** PORT ACTIONS **/

    /**
     * @brief Deletes a dynamic port
     * @param obj
     * @param type
     * @param idx
     */
    static void deleteDynamicPort(Node* obj, PortType type, PortIndex idx);

    /**
     * @brief Similar to `toDynamicNode`. Can be used for validation of a port
     * action
     * @param obj Object to cast
     * @return node object (may be null)
     */
    static bool isDynamicPort(GtObject* obj, PortType type, PortIndex idx);
    static bool isDynamicNode(GtObject* obj, PortType, PortIndex);

    /**
     * @brief Returns the list of all port actions registered
     * @return
     */
    QList<PortUIAction> const& portActions() const;

protected:

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

    /**
     * @brief Clears the intelli graph (i.e. removes all nodes and connections)
     * @param obj Intelli graph to clear
     */
    static void clearGraphNode(GtObject* obj);

    static bool deleteDummyNode(Node* node);

    static void duplicateGraph(GtObject* obj);

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

#endif // GT_INTELLI_NODEUI_H
