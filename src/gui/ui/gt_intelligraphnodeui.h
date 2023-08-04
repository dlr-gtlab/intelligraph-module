/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 14.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTINTELLIGRAPHNODEUI_H
#define GTINTELLIGRAPHNODEUI_H

#include "gt_objectui.h"
#include "gt_igportuiaction.h"
#include "gt_intelligraph_exports.h"

class GtIntelliGraph;
class GtIntelliGraphNode;
class GtIntelliGraphDynamicNode;

class GT_IG_EXPORT GtIntelliGraphNodeUI : public GtObjectUI
{
    Q_OBJECT

public:

    /// Option enum, can be used to deactive certain default actions
    enum Option
    {
        NoOption = 0,
        /// Deactivates the default port actions for dynamic nodes
        NoDefaultPortActions = 1,
    };

    using PortActionFunction = typename GtIgPortUIAction::ActionMethod;
    using PortType = gt::ig::PortType;
    using PortIndex = gt::ig::PortIndex;

    Q_INVOKABLE GtIntelliGraphNodeUI(Option option = NoOption);

    /**
     * @brief Icon for the object
     * @param obj Object
     * @return icon
     */
    QIcon icon(GtObject* obj) const override;

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
    static GtIntelliGraphNode* toNode(GtObject* obj);

    /**
     * @brief Returns the list of all port actions registered
     * @return
     */
    QList<GtIgPortUIAction> const& portActions() const { return m_portActions; }

    /** DYNAMIC NODES **/

    /**
     * @brief Casts the object to a dynamic node object. Can be used for
     * validation
     * @param obj Object to cast
     * @return node object (may be null)
     */
    static GtIntelliGraphDynamicNode* toDynamicNode(GtObject* obj);

    /**
     * @brief Similar to `toDynamicNode`. Can be used for validation of a port
     * action
     * @param obj Object to cast
     * @return node object (may be null)
     */
    static GtIntelliGraphDynamicNode* isDynamicNode(GtObject* obj, PortType, PortIndex);

protected:

    /**
     * @brief Adds a port action and returns a reference to the added action,
     * which can be used to customize the action. Reference may become invalid
     * if another port action is added.
     * @param actionText Text of action
     * @param actionMethod Method to execute
     * @return Reference to port action
     */
    GtIgPortUIAction& addPortAction(QString const& actionText,
                                    PortActionFunction actionMethod);

private:

    /// List of custom port actions
    QList<GtIgPortUIAction> m_portActions;

    /**
     * @brief Casts the object to an intelligraph object. Can be used for
     * validation
     * @param obj Object to check
     * @return intelligraph object ((may be null))
     */
    static GtIntelliGraph* toGraph(GtObject* obj);

    /**
     * @brief Prompts the user to rename the node
     * @param obj Object to rename
     */
    static void renameNode(GtObject* obj);

    /**
     * @brief Clears the intelli graph (i.e. removes all nodes and connections)
     * @param obj Intelli graph to clear
     */
    static void clearNodeGraph(GtObject* obj);

    /**
     * @brief Checks if node can be renamed (i.e. node should be valid but not unique)
     * @param obj Object to check
     * @return is object renamable
     */
    static bool canRenameNodeObject(GtObject* obj);

    /**
     * @brief loadNodeGraph
     * @param obj
     */
    static void loadNodeGraph(GtObject* obj);

    /** DYNAMIC NODES **/

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
    static void deleteDynamicPort(GtIntelliGraphNode* obj, PortType type, PortIndex idx);
};

#endif // GTINTELLIGRAPHNODEUI_H
