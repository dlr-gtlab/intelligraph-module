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

    enum Option
    {
        NoOption = 0,
        NoDefaultPortActions = 1,
    };

    using PortActionFunction = typename GtIgPortUIAction::ActionMethod;
    using PortType = gt::ig::PortType;
    using PortIndex = gt::ig::PortIndex;

    Q_INVOKABLE GtIntelliGraphNodeUI(Option option = NoOption);

    QIcon icon(GtObject* obj) const override;

    /**
     * @brief Returns the list of mdi items to open the object with
     * @param obj Object to open
     * @return Class names of compatible mdi items
     */
    QStringList openWith(GtObject* obj) override;

    /**
     * @brief Returns true if the object is a node object
     * @param obj Object to check
     * @return Is node
     */
    static GtIntelliGraphNode* toNode(GtObject* obj);

    QList<GtIgPortUIAction> const& portActions() const { return m_portActions; }

    /** DYNAMIC NODES **/

    static GtIntelliGraphDynamicNode* toDynamicNode2(GtObject* obj, PortType, PortIndex);
    static GtIntelliGraphDynamicNode* toDynamicNode(GtObject* obj) { return toDynamicNode2(obj, {}, PortIndex{}); }

protected:

    GtIgPortUIAction& addPortAction(QString const& actionText,
                                    PortActionFunction actionMethod);

private:

    /// List of custom port actions
    QList<GtIgPortUIAction> m_portActions;

    /**
     * @brief Returns true if the object is an intelli graph object
     * @param obj Object to check
     * @return Is intelli graph
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

    static void addInPort(GtObject* obj);

    static void addOutPort(GtObject* obj);

    static void deleteDynamicPort(GtIntelliGraphNode* obj, PortType type, PortIndex idx);
};

#endif // GTINTELLIGRAPHNODEUI_H
