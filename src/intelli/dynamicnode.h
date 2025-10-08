/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_DYNAMICNODE_H
#define GT_INTELLI_DYNAMICNODE_H

#include <intelli/node.h>

class GtPropertyStructInstance;

namespace intelli
{

/**
 * @brief The GtIntelliGraphDynamicNode class.
 * Extends the basic GtIntelliGraphNode class with the feature to store
 * ports, that have been added at runtime persistently.
 */
class GT_INTELLI_EXPORT DynamicNode : public Node
{
    Q_OBJECT

public:

    ~DynamicNode();

    /// Option for the node creation.
    enum Option : size_t
    {
        /// no ports can be added dynamically. Ports are not saved persistently.
        NoDynamicPorts = 0,
        /// input ports may be added dynamically (output ports may still be
        /// added, but wont be saved persistently)
        DynamicInput = 1 << 0,
        DynamicInputOnly [[deprecated("Use `DynamicOutput` instead")]] = 1 << 2,
        /// output ports may be added dynamically
        DynamicOutput = 1 << 1,
        DynamicOutputOnly [[deprecated("Use `DynamicOutput` instead")]] = 1 << 2,
        /// both inputs and ouput ports can be added dynamically
        DynamicInputAndOutput = DynamicInput | DynamicOutput,
        /// input ports can only be added programmatically
        /// (no UI action for adding/deleting port is added by default)
        NoUserDynamicInput  = 1 << 2,
        /// output ports can only be added programmatically
        /// (no UI action for adding/deleting port is added by default)
        NoUserDynamicOutput = 1 << 3,
        /// input and output ports may only be added programmatically
        /// (no UI action for adding/deleting port is added by default)
        NoUserDynamicInputAndOutput = NoUserDynamicInput | NoUserDynamicOutput
    };

    /**
     * @brief Getter for the node option used
     * @return
     */
    size_t dynamicNodeOption() const;

    /**
     * @brief Retruns true if a port is considered dynamic (i.e. was added at
     * runtime and may be deleted by the user)
     * @param type Port type
     * @param idx Port Index to check
     * @return Is port dynamic
     */
    bool isDynamicPort(PortType type, PortIndex idx) const;

    /**
     * @brief Appends a dynamic/user input port. User ports are saved
     * persistently and may be added, modifed or removed at runtime. Ideally
     * these should only be outside the constructor scope.
     * @param port Port to append
     * @param policy Port policy
     * @return Port id
     */
    PortId addInPort(PortInfo port, PortPolicy policy = DefaultPortPolicy);

    /**
     * @brief Appends a dynamic/user output port. User ports are saved
     * persistently and may be added, modifed or removed at runtime. Ideally
     * these should only be outside the constructor scope.
     * @param port Port to append
     * @param policy Port policy
     * @return Port id
     */
    PortId addOutPort(PortInfo port);

    /**
     * @brief Inserts a user input port at the given position.
     * @param port Port to insert
     * @param idx Where to insert the port. The port will always be inserted
     * behind the last static port if any was registered.
     * @param policy Port policy
     * @return Port id
     */
    PortId insertInPort(PortInfo port,int idx = -1, PortPolicy policy = DefaultPortPolicy);

    /**
     * @brief Inserts a user output port at the given position.
     * @param port Port to insert
     * @param idx Where to insert the port. The port will always be inserted
     * behind the last static port if any was registered.
     * @param policy Port policy
     * @return Port id
     */
    PortId insertOutPort(PortInfo port, int idx = -1);
    
    using Node::removePort;

protected:

    enum PortOption
    {
        StaticPort = 0,
        DynamicPort,
    };

    /**
     * @brief constructor. Must initialize the model name.
     * @param modelName Model name. May not be altered later
     * @param option Option for dynamic node. May be used to allow
     * only dynamic input or output ports (default is both)
     * @param parent Parent object
     */
    DynamicNode(QString const& modelName,
                size_t option = DynamicInputAndOutput,
                GtObject* parent = nullptr);

    DynamicNode(QString const& modelName,
                QStringList inputWhiteList,
                QStringList outputWhiteList,
                size_t option = DynamicInputAndOutput,
                GtObject* parent = nullptr);

    /**
     * @brief Appends a static input port. Static ports are like the regular
     * ports and are not saved persistently. They should only be created within
     * the constructor scope. The port will always be inserted
     * before any dynamic port.
     * @param port Port to append
     * @param policy Port policy
     * @return Port id
     */
    PortId addStaticInPort(PortInfo port, PortPolicy policy = DefaultPortPolicy);

    /**
     * @brief Appends a static output port. Static ports are like the regular
     * ports and are not saved persistently. They should only be created within
     * the constructor scope. The port will always be inserted
     * before any dynamic port.
     * @param port Port to append
     * @return Port id
     */
    PortId addStaticOutPort(PortInfo port);

    /**
     * @brief Common helper method for inserting a port at a given location.
     * It can both insert a static and dynamic/user port
     * @param option Port option (static or dynamic)
     * @param type Port type
     * @param port Port to insert
     * @param idx Where to insert the port
     * @return Port id
     */
    virtual PortId insertPort(PortOption option, PortType type, PortInfo port, int idx = -1);

    /**
     * @brief Can be used to override the portId used for inserting a new port
     * @param portId PortId to verify
     * @return New Port id
     */
    virtual PortId verifyPortId(PortId portId);

private slots:

    /**
     * @brief Removes the property container entry for the removed port
     * @param type
     * @param idx
     */
    void onPortDeleted(PortType type, PortIndex idx);

    /**
     * @brief Inserts the port described by the property container entry to
     * the node
     * @param idx
     */
    void onPortEntryAdded(int idx);

    /**
     * @brief Updates the given port
     * @param idx
     * @param property
     */
    void onPortEntryChanged(int idx, GtAbstractProperty* = nullptr);

    /**
     * @brief Removes the given port
     * @param idx
     */
    void onPortEntryRemoved(int idx);

private:

    // hide defaul add/insert/remove port method in favor of custom methods
    using Node::addInPort;
    using Node::addOutPort;
    using Node::insertInPort;
    using Node::insertOutPort;
    using Node::insertPort;

    struct Impl;
    std::unique_ptr<Impl> pimpl;

    /**
     * @brief Returns the offset for to the first index of a dynamic port.
     * @param type Port type to get offset from
     * @return Offset to beginning of the first index of a dynamic port.
     */
    int offset(PortType type) const;

    /**
     * @brief Can be used to check which port tyoe the container belongs to
     * @param container Container to get port type from
     * @return Port Type (NoType if the container does not match any member)
     */
    PortType toPortType(GtPropertyStructContainer const& container) const;

    /**
     * @brief Access the property container that belonges to the speicifed port
     * type. Will throw if an invalid port type was specified
     * @param type Port type
     * @return Property container
     */
    GtPropertyStructContainer& dynamicPorts(PortType type) noexcept(false);
    GtPropertyStructContainer const& dynamicPorts(PortType type) const noexcept(false);

    /**
     * @brief Checks if the QObject ptr matches any property container and
     * returns said container. Useful mapping the sender of a singal to a
     * property container
     * @param sender
     * @return Property container matched by obj. Null if the pointer does not
     * match any property containers
     */
    GtPropertyStructContainer* toDynamicPorts(QObject* obj);

    /**
     * @brief Exception safe helper method for accessing the property entry
     * denoted by idx
     * @param container Container to access
     * @param idx Index to access
     * @return Pointer to entry (may be null if index does not match a valid entry)
     */
    static GtPropertyStructInstance* propertyAt(GtPropertyStructContainer* container, int idx);
};

} // namespace intelli

#endif // GT_INTELLI_DYNAMICNODE_H
