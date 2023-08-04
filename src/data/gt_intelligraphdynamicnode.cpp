/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphdynamicnode.h"
#include "gt_igstringselectionproperty.h"
#include "gt_intelligraphdatafactory.h"

#include "private/utils.h"

#include "gt_structproperty.h"
#include "gt_stringproperty.h"
#include "gt_boolproperty.h"
#include "gt_intproperty.h"
#include "gt_exceptions.h"

QString const S_PORT_DATA = QStringLiteral("PortData");

QString const S_PORT_TYPE = QStringLiteral("TypeId");
QString const S_PORT_CAPTION = QStringLiteral("Caption");
QString const S_PORT_CAPTION_VISIBLE = QStringLiteral("CaptionVisible");
QString const S_PORT_OPTIONAL = QStringLiteral("Optional");
QString const S_PORT_ID = QStringLiteral("PortId");

GtIntelliGraphDynamicNode::GtIntelliGraphDynamicNode(QString const& modelName,
                                                     DynamicNodeOption option,
                                                     GtObject* parent) :
    GtIntelliGraphNode(modelName, parent),
    m_inPorts("dynamicInPorts", "In Ports"),
    m_outPorts("dynamicOutPorts", "Out Ports"),
    m_option(option)
{
    GtPropertyStructDefinition portData{S_PORT_DATA};

    auto makeReadOnly = [](auto func){
        return [func = std::move(func)](QString const& id){
            GtAbstractProperty* tmp = func(id);
            tmp->setReadOnly(true);
            return tmp;
        };
    };

    auto typeIds = GtIntelliGraphDataFactory::instance().registeredTypeIds();
    portData.defineMember(S_PORT_TYPE, gt::ig::makeStringSelectionProperty(typeIds));
    portData.defineMember(S_PORT_CAPTION, gt::makeStringProperty());
    portData.defineMember(S_PORT_CAPTION_VISIBLE, gt::makeBoolProperty(true));
    portData.defineMember(S_PORT_OPTIONAL, gt::makeBoolProperty(true));
    portData.defineMember(S_PORT_ID, makeReadOnly(gt::makeIntProperty(999)));

    m_inPorts.registerAllowedType(portData);
    m_outPorts.registerAllowedType(portData);

    if (m_option != DynamicOutputOnly) registerPropertyStructContainer(m_inPorts);
    if (m_option != DynamicInputOnly)  registerPropertyStructContainer(m_outPorts);

    connect(this, &GtIntelliGraphNode::portInserted,
            this, &GtIntelliGraphDynamicNode::onPortInserted,
            Qt::UniqueConnection);
    connect(this, &GtIntelliGraphNode::portAboutToBeDeleted,
            this, &GtIntelliGraphDynamicNode::onPortDeleted,
            Qt::UniqueConnection);

    for (auto* ports : { &m_inPorts, &m_outPorts})
    {
        connect(ports, &GtPropertyStructContainer::entryAdded,
                this, &GtIntelliGraphDynamicNode::onPortEntryAdded,
                Qt::UniqueConnection);
        connect(ports, &GtPropertyStructContainer::entryChanged,
                this, &GtIntelliGraphDynamicNode::onPortEntryChanged,
                Qt::UniqueConnection);
        connect(ports, &GtPropertyStructContainer::entryRemoved,
                this, &GtIntelliGraphDynamicNode::onPortEntryRemoved,
                Qt::UniqueConnection);
    }
}

GtIntelliGraphDynamicNode::DynamicNodeOption
GtIntelliGraphDynamicNode::dynamicNodeOption() const
{
    return m_option;
}

void
GtIntelliGraphDynamicNode::onPortInserted(PortType type, PortIndex idx)
{
    auto portId = this->portId(type, idx);
    auto* port = this->port(portId);
    if (!port)
    {
        gtWarning() << tr("Adding dynamic port failed! (Port '%1' not found, type: %2)")
                           .arg(portId).arg(type);
        return;
    }

    auto& ports = dynamicPorts(type);

    // ignore changed signals of property container
    auto ignoreAdded = ignoreSignal(
        &ports, &GtPropertyStructContainer::entryAdded,
        this, &GtIntelliGraphDynamicNode::onPortEntryAdded
    );
    auto ignoreChanged = ignoreSignal(
        &ports, &GtPropertyStructContainer::entryChanged,
        this, &GtIntelliGraphDynamicNode::onPortEntryChanged
    );
    Q_UNUSED(ignoreAdded);
    Q_UNUSED(ignoreChanged);

    auto iter = std::find_if(ports.begin(), ports.end(), [=](auto const& iter){
        bool ok = true;
        auto id = PortId::fromValue(iter.template getMemberVal<int>(S_PORT_ID, &ok));
        return ok && id == portId;
    });

    if (iter != ports.end()) return;

    gtInfo().verbose() << tr("Adding dynamic port entry:") << *port;

    auto& entry = ports.newEntry(S_PORT_DATA, std::next(ports.begin(), idx));
    entry.setMemberVal(S_PORT_ID, portId.value());
    entry.setMemberVal(S_PORT_TYPE, port->typeId);
    entry.setMemberVal(S_PORT_CAPTION, port->caption);
    entry.setMemberVal(S_PORT_CAPTION_VISIBLE, port->captionVisible);
    entry.setMemberVal(S_PORT_OPTIONAL, port->optional);
}

void
GtIntelliGraphDynamicNode::onPortDeleted(PortType type, PortIndex idx)
{
    auto portId = this->portId(type, idx);
    if (portId == gt::ig::invalid<PortId>())
    {
        gtWarning() << tr("Removing dynamic port failed! (Port '%1' not found, type: %2)")
                           .arg(portId).arg(type);
        return;
    }

    auto& ports = dynamicPorts(type);

    // ignore removed signal of property container
    auto ignoreRemoved = ignoreSignal(
        &ports, &GtPropertyStructContainer::entryRemoved,
        this, &GtIntelliGraphDynamicNode::onPortEntryRemoved
    );
    Q_UNUSED(ignoreRemoved);

    auto iter = std::find_if(ports.begin(), ports.end(), [=](auto const& iter){
        bool ok = true;
        auto id = PortId::fromValue(iter.template getMemberVal<int>(S_PORT_ID, &ok));
        return ok && id == portId;
    });

    if (iter == ports.end()) return;

    gtInfo().verbose() << tr("Removing dynamic port entry:") << portId;

    ports.removeEntry(iter);
}

void
GtIntelliGraphDynamicNode::onPortEntryAdded(int idx)
{
    if (!m_merged) return;

    auto* ports = toDynamicPorts(sender());
    auto* entry = propertyAt(ports, idx);
    if (!entry) return;

    addPortFromEntry(*entry, toPortType(*ports));
}

void
GtIntelliGraphDynamicNode::onPortEntryChanged(int idx, GtAbstractProperty*)
{
    if (!m_merged) return;

    auto* ports = toDynamicPorts(sender());
    auto* entry = propertyAt(ports, idx);
    if (!entry) return;

    auto portId = PortId::fromValue(entry->template getMemberVal<int>(S_PORT_ID));
    auto* port = this->port(portId);
    if (!port)
    {
        gtWarning() << tr("Updating dynamic port entry failed! (Port idx '%1' not found)")
                           .arg(idx);
        return;
    }

    gtInfo().verbose() << tr("Updating dynamic port entry:") << *port;

    QString typeId  = entry->template getMemberVal<QString>(S_PORT_TYPE);
    QString caption = entry->template getMemberVal<QString>(S_PORT_CAPTION);
    bool captionVisible = entry->template getMemberVal<bool>(S_PORT_CAPTION_VISIBLE);
    bool optional = entry->template getMemberVal<bool>(S_PORT_OPTIONAL);

    port->typeId = std::move(typeId);
    port->caption = std::move(caption);
    port->captionVisible = captionVisible;
    port->optional = optional;

    emit portChanged(port->id());
}

void
GtIntelliGraphDynamicNode::onPortEntryRemoved(int idx)
{
    auto* ports = toDynamicPorts(sender());
    if (!ports) return;

    auto type = toPortType(*ports);
    PortId portId = this->portId(type, PortIndex::fromValue(idx));

    if (portId == gt::ig::invalid<PortId>())
    {
        gtWarning() << tr("Removing dynamic port entry failed! (Port idx '%1' not found)")
                           .arg(idx);
        return;
    }

    // ignore removed signal
    auto ignoreRemoved = ignoreSignal(
        this, &GtIntelliGraphNode::portAboutToBeDeleted,
        this, &GtIntelliGraphDynamicNode::onPortDeleted
    );
    Q_UNUSED(ignoreRemoved);

    removePort(portId);
}

void
GtIntelliGraphDynamicNode::addPortFromEntry(GtPropertyStructInstance& entry, PortType type)
{
    QString typeId  = entry.template getMemberVal<QString>(S_PORT_TYPE);
    QString caption = entry.template getMemberVal<QString>(S_PORT_CAPTION);
    bool captionVisible = entry.template getMemberVal<bool>(S_PORT_CAPTION_VISIBLE);
    bool optional = entry.template getMemberVal<bool>(S_PORT_OPTIONAL);
    auto portId = PortId::fromValue(entry.template getMemberVal<int>(S_PORT_ID));

    auto ignoreInserted = ignoreSignal(
        this, &GtIntelliGraphNode::portInserted,
        this, &GtIntelliGraphDynamicNode::onPortInserted
    );
    Q_UNUSED(ignoreInserted);

    if (auto* p = port(portId))
    {
        gtWarning().nospace() << tr("Adding dynamic port entry failed! "
                                    "(Port already exists: ") << *p << ")";
        return;
    }

    PortData portData = { typeId, caption, captionVisible, optional };

    portId = insertPort(type, portData);

    entry.setMemberVal(S_PORT_ID, portId.value());
}

GtIntelliGraphNode::PortType
GtIntelliGraphDynamicNode::toPortType(GtPropertyStructContainer const& container) const
{
    if (&container == &m_inPorts) return PortType::In;
    if (&container == &m_outPorts) return PortType::Out;
    return PortType::NoType;
}

GtPropertyStructContainer&
GtIntelliGraphDynamicNode::dynamicPorts(PortType type) noexcept(false)
{
    switch (type)
    {
    case PortType::In:
        return m_inPorts;
    case PortType::Out:
        return m_outPorts;
    case PortType::NoType:
        break;
    }

    throw GTlabException{
        __FUNCTION__, QStringLiteral("Invalid port type specified!")
    };
}

GtPropertyStructContainer*
GtIntelliGraphDynamicNode::toDynamicPorts(QObject* obj)
{
    if (obj == &m_inPorts)  return &m_inPorts;
    if (obj == &m_outPorts) return &m_outPorts;
    return nullptr;
}

GtPropertyStructInstance*
GtIntelliGraphDynamicNode::propertyAt(GtPropertyStructContainer* container, int idx)
{
    if (!container) return nullptr;

    try
    {
        return &container->at(idx);
    }
    catch (std::out_of_range const& e)
    {
        gtError().nospace() << __FUNCTION__ << ":" << e.what();
    }
    return nullptr;
}

void
GtIntelliGraphDynamicNode::onObjectDataMerged()
{
    if (!m_merged)
    {
        for (auto* ports : { &m_inPorts, &m_outPorts})
        {
            for (auto& entry : *ports)
            {
                addPortFromEntry(entry, toPortType(*ports));
            }
        }
        m_merged = true;
    }
    return GtIntelliGraphNode::onObjectDataMerged();
}
