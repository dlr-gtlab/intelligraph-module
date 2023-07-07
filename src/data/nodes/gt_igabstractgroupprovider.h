/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_IGABSTRACTGROUPPROVIDER_H
#define GT_IGABSTRACTGROUPPROVIDER_H

#include "gt_coreapplication.h"
#include "gt_intelligraphnode.h"
#include "gt_intelligraphdatafactory.h"
#include "gt_igstringselectionproperty.h"

#include "gt_propertystructcontainer.h"
#include "gt_structproperty.h"
#include "gt_intelligraph.h"

template <gt::ig::PortType Type>
class GtIgAbstractGroupProvider : public GtIntelliGraphNode
{
public:

    static QString const S_STRUCT_ID;
    static QString const S_TYPEID;
    static QString const S_CAPTION;

    static constexpr inline PortType INVERSE_TYPE() noexcept
    {
        return Type == PortType::In ? PortType::Out : PortType::In;
    }

    static constexpr inline PortType TYPE() noexcept
    {
        return Type;
    }

    GtIgAbstractGroupProvider(QString const& modelName) :
        GtIntelliGraphNode(modelName),
        m_ports("ports", tr("Port Types"))
    {
        setId(NodeId{TYPE()});
        setFlag(UserDeletable, false);

        if (!gtApp || !gtApp->devMode()) setFlag(UserHidden, true);

        GtPropertyStructDefinition portEntry{S_STRUCT_ID};

        auto allowedTypes = GtIntelliGraphDataFactory::instance().knownClasses();
        portEntry.defineMember(S_TYPEID,  gt::ig::makeStringSelectionProperty(std::move(allowedTypes)));
        portEntry.defineMember(S_CAPTION, gt::makeStringProperty());

        m_ports.registerAllowedType(portEntry);

        registerPropertyStructContainer(m_ports);

        connect(&m_ports, &GtPropertyStructContainer::entryAdded,
            this, &GtIgAbstractGroupProvider::onEntryAdded);

        connect(&m_ports, &GtPropertyStructContainer::entryRemoved,
            this, &GtIgAbstractGroupProvider::onEntryRemoved);

        connect(&m_ports, &GtPropertyStructContainer::entryChanged,
                this, &GtIgAbstractGroupProvider::onEntryChanged);
    }

    void insertPort(PortData data, int idx)
    {
        auto& ports = this->ports(INVERSE_TYPE());

        if (idx < 0 || static_cast<size_t>(idx) > ports.size())
        {
            idx = ports.size();
        }

        auto& entry = m_ports.newEntry(S_STRUCT_ID, std::next(m_ports.begin(), idx));
        entry.setMemberVal(S_TYPEID,  data.typeId);
        entry.setMemberVal(S_CAPTION, data.caption);
    }

    void removePort(int idx)
    {
        if (idx < 0 || static_cast<size_t>(idx) >= m_ports.size()) return;

        m_ports.removeEntry(std::next(m_ports.begin(), idx));
    }

protected:

    GtPropertyStructContainer m_ports;

private:

    // "hide" unused methods
    using GtIntelliGraphNode::addInPort;
    using GtIntelliGraphNode::addOutPort;
    using GtIntelliGraphNode::insertInPort;
    using GtIntelliGraphNode::insertOutPort;
    using GtIntelliGraphNode::removePort;

private slots:

    GtPropertyStructInstance* propertyAt(int idx)
    {
        try
        {
            return &m_ports.at(idx);
        }
        catch (std::out_of_range const& e)
        {
            gtError() << e.what();
        }
        return nullptr;
    }

    void onEntryAdded(int idx)
    {
        auto const* entry = propertyAt(idx);
        if (!entry) return;

        QString typeId  = entry->template getMemberVal<QString>(S_TYPEID);
        QString caption = entry->template getMemberVal<QString>(S_CAPTION);

        gtDebug().verbose() << "Adding port" << typeId
                            << gt::brackets("caption: " + gt::squoted(caption));

        Type == PortType::In ?
            insertOutPort({typeId, caption}, idx) :
            insertInPort({typeId, caption}, idx);

        onPortInserted(INVERSE_TYPE(), PortIndex::fromValue(idx));
    }

    void onEntryRemoved(int idx)
    {
        PortId id = portId(INVERSE_TYPE(), PortIndex::fromValue(idx));
        if (id == gt::ig::invalid<PortId>())
        {
            gtWarning() << tr("Failed to remove port idx '%1'!").arg(idx);
            return;
        }

        gtDebug().verbose() << "Removing port idx" << idx;

        GtIntelliGraphNode::removePort(id);

        onPortDeleted(PortIndex::fromValue(idx));
    }

    void onEntryChanged(int idx, GtAbstractProperty*)
    {
        PortId id   = portId(INVERSE_TYPE(), PortIndex::fromValue(idx));
        PortData* p = port(id);
        if (!p)
        {
            gtWarning() << tr("Failed to access port idx '%1'!").arg(idx);
            return;
        }

        auto const* entry = propertyAt(idx);
        if (!entry) return;

        QString typeId  = entry->template getMemberVal<QString>(S_TYPEID);
        QString caption = entry->template getMemberVal<QString>(S_CAPTION);

        gtDebug().verbose() << "Updating port" << idx << "to" << typeId
                            << gt::brackets("caption: " + caption);

        p->typeId = typeId;
        p->caption = caption;

        emit portChanged(id);

        onPortChanged(id);
    }

    void onPortInserted(PortType type, PortIndex idx)
    {
        auto* graph = findParent<GtIntelliGraph*>();
        if (!graph) return;

        PortId id = portId(INVERSE_TYPE(), idx);
        if (auto* port = this->port(id))
        {
            TYPE() == PortType::In ?
                graph->insertInPort(*port, idx) :
                graph->insertOutPort(*port, idx);

            if (TYPE() == PortType::Out)
            {
                graph->insertOutData(idx);
            }
        }
    }

    void onPortChanged(PortId id)
    {
        auto* graph = findParent<GtIntelliGraph*>();
        if (!graph) return;

        auto* inPort = port(id);
        auto  idx    = portIndex(INVERSE_TYPE(), id);
        auto* port   = graph->port(graph->portId(TYPE(), idx));

        if (!inPort || !port) return;

        port->typeId = inPort->typeId;
        port->caption = inPort->caption;
        emit graph->portChanged(port->id());
    }

    void onPortDeleted(PortIndex idx)
    {
        auto* graph = findParent<GtIntelliGraph*>();
        if (!graph) return;

        graph->removePort(graph->portId(TYPE(), idx));
    }
};

// disbale template class for none type
template <> class GtIgAbstractGroupProvider<gt::ig::NoType>;

template <gt::ig::PortType type>
QString const GtIgAbstractGroupProvider<type>::S_STRUCT_ID = QStringLiteral("Port");
template <gt::ig::PortType type>
QString const GtIgAbstractGroupProvider<type>::S_TYPEID = QStringLiteral("type_id");
template <gt::ig::PortType type>
QString const GtIgAbstractGroupProvider<type>::S_CAPTION = QStringLiteral("caption");

#endif // GT_IGABSTRACTGROUPPROVIDER_H
