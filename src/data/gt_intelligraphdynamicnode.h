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

class GtIntelliGraphDynamicNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    using GtIntelliGraphNode::addInPort;
    using GtIntelliGraphNode::addOutPort;
    using GtIntelliGraphNode::insertInPort;
    using GtIntelliGraphNode::insertOutPort;
    using GtIntelliGraphNode::removePort;

protected:

    /**
     * @brief constructor. Must initialize the model name.
     * @param modelName Model name. May not be altered later
     * @param parent Parent object
     */
    GtIntelliGraphDynamicNode(QString const& modelName, GtObject* parent = nullptr);

private slots:

    void onPortInserted(PortType type, PortIndex idx);

    void onPortDeleted(PortType type, PortIndex idx);

    void onPortEntryAdded(int idx);

    void onPortEntryChanged(int idx, GtAbstractProperty* property);

    void onPortEntryRemoved(int idx);

private:

    GtPropertyStructContainer m_inPorts;
    GtPropertyStructContainer m_outPorts;

    PortType toPortType(GtPropertyStructContainer const& container) const;

    GtPropertyStructContainer& dynamicPorts(PortType type);

    GtPropertyStructContainer* dynamicPorts(QObject* sender);

    static GtPropertyStructInstance* propertyAt(GtPropertyStructContainer* container, int idx);
};

#endif // GTINTELLIGRAPHDYNAMICNODE_H
