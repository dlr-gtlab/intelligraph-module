/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 14.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_NODEUI_H
#define GT_INTELLI_NODEUI_H

#include <intelli/gui/portuiaction.h>
#include <intelli/exports.h>



#include <gt_objectui.h>


namespace intelli
{

class Graph;
class Node;
class NodeGraphicsObject;
class DynamicNode;

class NodeGeometry
{
public:

    NodeGeometry(Node& node);
    NodeGeometry(NodeGeometry const&) = delete;
    NodeGeometry(NodeGeometry&&) = delete;
    NodeGeometry& operator=(NodeGeometry const&) = delete;
    NodeGeometry& operator=(NodeGeometry&&) = delete;
    virtual ~NodeGeometry() = default;

    struct PortHit
    {
        PortType type{PortType::NoType};
        PortId port{};

        operator bool() const
        {
            return type != PortType::NoType && port != invalid<PortId>();
        }
    };

    bool positionWidgetAtBottom() const;

    int hspacing() const;
    int vspacing() const;

    QPainterPath shape() const;

    QRectF innerRect() const;

    QRectF boundingRect() const;

    QRectF captionRect() const;

    QPointF evalStateVisualizerPosition() const;

    QPointF widgetPosition() const;

    QRectF portRect(PortType type, PortIndex idx) const;

    QRectF portCaptionRect(PortType type, PortIndex idx) const;

    PortHit portHit(QPointF coord) const;
    PortHit portHit(QRectF coord) const;

    QRectF resizeHandleRect() const;

    void recomputeGeomtry();

private:
    Node* m_node;
    mutable bool m_isCalculating = false;

    int captionHeightExtend() const;

    int portHorizontalExtend(PortType type) const;

    int portHeightExtend() const;
};

class NodePainter
{
public:

    NodePainter(NodeGraphicsObject& obj, NodeGeometry& geometry);
    NodePainter(NodePainter const&) = delete;
    NodePainter(NodePainter&&) = delete;
    NodePainter& operator=(NodePainter const&) = delete;
    NodePainter& operator=(NodePainter&&) = delete;
    virtual ~NodePainter() = default;

    QColor backgroundColor() const;

    void drawRect(QPainter& painter);
    void drawPorts(QPainter& painter);
    void drawCaption(QPainter& painter);

    void paint(QPainter& painter);

private:
    NodeGraphicsObject* m_object;
    NodeGeometry* m_geometry;
};

class GT_INTELLI_EXPORT NodeUI : public GtObjectUI
{
    Q_OBJECT

public:

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
    
    virtual std::unique_ptr<NodePainter> painter(NodeGraphicsObject& object, NodeGeometry& geometry) const;
    
    virtual std::unique_ptr<NodeGeometry> geometry(Node& node) const;

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
    static Node* toNode(GtObject* obj);

    /**
     * @brief Casts the object to an intelligraph object. Can be used for
     * validation
     * @param obj Object to check
     * @return intelligraph object (may be null)
     */
    static Graph* toGraph(GtObject* obj);

    /**
     * @brief Casts the object to a dynamic node object. Can be used for
     * validation
     * @param obj Object to cast
     * @return node object (may be null)
     */
    static DynamicNode* toDynamicNode(GtObject* obj);

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
    QList<PortUIAction> const& portActions() const { return m_portActions; }

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

private:

    /// List of custom port actions
    QList<PortUIAction> m_portActions;

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
