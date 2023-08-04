/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTINTELLIGRAPHDYNAMICNODE_H
#define GTINTELLIGRAPHDYNAMICNODE_H

#include "gt_intelligraphnode.h"

#include "gt_propertystructcontainer.h"

/**
 * @brief The GtIntelliGraphDynamicNode class.
 * Extends the basic GtIntelliGraphNode class with the feature to store
 * ports, that have been added at runtime persistently.
 */
class GtIntelliGraphDynamicNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    // make add/insert/remove port functions public
    using GtIntelliGraphNode::addInPort;
    using GtIntelliGraphNode::addOutPort;
    using GtIntelliGraphNode::insertInPort;
    using GtIntelliGraphNode::insertOutPort;
    using GtIntelliGraphNode::removePort;

    /// Option for the node creation
    enum DynamicNodeOption
    {
        /// both inputs and ouput ports can be added dynamically
        DynamicInputAndOutput = 0,
        /// only input ports may be added dynamically (output ports may still be
        /// added, but wont be saved persistently)
        DynamicInputOnly,
        /// only output ports may be added dynamically (input ports may still be
        /// added, but wont be saved persistently)
        DynamicOutputOnly
    };

    /**
     * @brief Getter for the node option used at creation
     * @return
     */
    DynamicNodeOption dynamicNodeOption() const;

protected:

    /**
     * @brief constructor. Must initialize the model name.
     * @param modelName Model name. May not be altered later
     * @param option Option for dynamic node. May be used to allow
     * only dynamic input or output ports (default is both)
     * @param parent Parent object
     */
    GtIntelliGraphDynamicNode(QString const& modelName,
                              DynamicNodeOption option = {},
                              GtObject* parent = nullptr);

    /**
     * @brief Used to eliminate duplicate registration of nodes in the constructor
     * and when the first memento was merged
     */
    void onObjectDataMerged() override;

private slots:

    /**
     * @brief Will add the property container entry for the added port
     * @param type
     * @param idx
     */
    void onPortInserted(PortType type, PortIndex idx);

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
    void onPortEntryChanged(int idx, GtAbstractProperty* property);

    /**
     * @brief Removes the given port
     * @param idx
     */
    void onPortEntryRemoved(int idx);

private:

    /// property container for the in ports
    GtPropertyStructContainer m_inPorts;
    /// property container for the out ports
    GtPropertyStructContainer m_outPorts;

    /// Node option
    DynamicNodeOption m_option = {};

    /// inidcates whether the port from the very first memento just after
    /// instantiation were merged. Used to prohibit duplicated registration of
    /// ports. (TODO: may pose problems when a node was not restored by memento)
    bool m_merged = false;

    /**
     * @brief Helper method to add the port for the property container entry
     * @param entry
     * @param type
     */
    void addPortFromEntry(GtPropertyStructInstance& entry, PortType type);

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

#endif // GTINTELLIGRAPHDYNAMICNODE_H
