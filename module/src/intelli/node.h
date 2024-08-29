/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_NODE_H
#define GT_INTELLI_NODE_H

#include <intelli/exports.h>

#include <intelli/lib/nodebase.h>

#include <gt_object.h>

#include <QWidget>

namespace intelli
{

class NodeExecutor;
class NodeData;
class NodeDataInterface;
class GraphExecutionModel;
struct NodeImpl;

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

class GT_INTELLI_EXPORT Node : public GtObject,
                               public lib::NodeBase
{
    Q_OBJECT
    
    friend class NodeExecutor;
    friend class NodeGraphicsObject;
    friend class GraphExecutionModel;

public:

    /// widget factory function type. Parameter is guranteed to be of type
    /// "this" and can be casted safely using static_cast.
    using WidgetFactory =
        std::function<std::unique_ptr<QWidget>(Node& thisNode)>;
    using WidgetFactoryNoArgs =
        std::function<std::unique_ptr<QWidget>()>;
    
    ~Node();

    Node& setActive(bool active = true)
    {
        lib::NodeBase::setActive(active);
        return *this;
    }

    Node& setId(NodeId id)
    {
        lib::NodeBase::setId(id);
        return *this;
    }

    Node& setPos(Position pos)
    {
        lib::NodeBase::setPos(pos);
        return *this;
    }

    Node& setSize(QSize size)
    {
        lib::NodeBase::setSize(size);
        return *this;
    }

    Node& setCaption(QString const& caption)
    {
        lib::NodeBase::setCaption(caption);
        return *this;
    }

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

    void onNodeChange(NodeChange change) override;

    void onPortsChange(PortsChange change, PortType type, PortIndex idx) override;

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
