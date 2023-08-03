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

GtIntelliGraphDynamicNode::GtIntelliGraphDynamicNode(const QString& modelName, GtObject* parent) :
    GtIntelliGraphNode(modelName, parent),
    m_inPorts("dynamicInPorts", "Dynamic In Ports"),
    m_outPorts("dynamicOutPorts", "Dynamic Out Ports")
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
    portData.defineMember(S_PORT_ID, makeReadOnly(gt::makeIntProperty(100)));

    m_inPorts.registerAllowedType(portData);
    m_outPorts.registerAllowedType(portData);

    registerPropertyStructContainer(m_inPorts);
    registerPropertyStructContainer(m_outPorts);

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

void
GtIntelliGraphDynamicNode::onPortInserted(PortType type, PortIndex idx)
{
    auto portId = this->portId(type, idx);
    auto* port = this->port(portId);
    if (!port)
    {
        gtWarning() << tr("Port Not found!") << type << idx << portId << __FUNCTION__;
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

    gtWarning() << tr("Adding Dynamic Port!") << type << *port;

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
        gtWarning() << tr("Port Not found!") << type << idx << portId << __FUNCTION__;
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

    ports.removeEntry(iter);
}

void
GtIntelliGraphDynamicNode::onPortEntryAdded(int idx)
{
    auto* ports = dynamicPorts(sender());
    auto* entry = propertyAt(ports, idx);
    if (!entry) return;

    QString typeId  = entry->template getMemberVal<QString>(S_PORT_TYPE);
    QString caption = entry->template getMemberVal<QString>(S_PORT_CAPTION);
    bool captionVisible = entry->template getMemberVal<bool>(S_PORT_CAPTION_VISIBLE);
    bool optional = entry->template getMemberVal<bool>(S_PORT_OPTIONAL);
    auto portId = PortId::fromValue(entry->template getMemberVal<int>(S_PORT_ID));

    auto ignoreInserted = ignoreSignal(
        this, &GtIntelliGraphNode::portInserted,
        this, &GtIntelliGraphDynamicNode::onPortInserted
    );
    Q_UNUSED(ignoreInserted);

    if (auto* p = port(portId))
    {
        gtInfo() << "port already exists!" << *p;
        return;
    }

    PortType type = toPortType(*ports);

    PortData portData = { typeId, caption, captionVisible, optional };

    gtDebug() << __FUNCTION__ << type << portData;

    portId = insertPort(type, portData);

    entry->setMemberVal(S_PORT_ID, portId.value());
}

void
GtIntelliGraphDynamicNode::onPortEntryChanged(int idx, GtAbstractProperty* property)
{
    auto* ports = dynamicPorts(sender());

    gtDebug() << __FUNCTION__ << idx << ports << property;
}

void
GtIntelliGraphDynamicNode::onPortEntryRemoved(int idx)
{
    auto* ports = dynamicPorts(sender());

    gtDebug() << __FUNCTION__ << idx << ports;
}

GtIntelliGraphNode::PortType
GtIntelliGraphDynamicNode::toPortType(GtPropertyStructContainer const& container) const
{
    if (&container == &m_inPorts) return PortType::In;
    if (&container == &m_outPorts) return PortType::Out;
    return PortType::NoType;
}

GtPropertyStructContainer&
GtIntelliGraphDynamicNode::dynamicPorts(PortType type)
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
GtIntelliGraphDynamicNode::dynamicPorts(QObject* sender)
{
    if (sender == &m_inPorts)  return &m_inPorts;
    if (sender == &m_outPorts)  return &m_outPorts;
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
        gtError() << e.what();
    }
    return nullptr;
}
