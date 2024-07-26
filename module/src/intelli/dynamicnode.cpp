/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/dynamicnode.h"
#include "intelli/property/stringselection.h"
#include "intelli/property/uint.h"
#include "intelli/nodedatafactory.h"
#include "intelli/private/utils.h"

#include "gt_structproperty.h"
#include "gt_stringproperty.h"
#include "gt_boolproperty.h"
#include "gt_exceptions.h"

using namespace intelli;

QString const S_PORT_INFO_IN  = QStringLiteral("PortInfoIn");
QString const S_PORT_INFO_OUT = QStringLiteral("PortInfoOut");

QString const S_PORT_TYPE = QStringLiteral("TypeId");
QString const S_PORT_CAPTION = QStringLiteral("Caption");
QString const S_PORT_CAPTION_VISIBLE = QStringLiteral("CaptionVisible");
QString const S_PORT_OPTIONAL = QStringLiteral("Optional");
QString const S_PORT_ID = QStringLiteral("PortId");

DynamicNode::DynamicNode(QString const& modelName,
                         Option option,
                         GtObject* parent) :
    DynamicNode(modelName, QStringList{}, QStringList{}, option, parent)
{ }

DynamicNode::DynamicNode(QString const& modelName,
                         QStringList inputWhiteList,
                         QStringList outputWhiteList,
                         Option option,
                         GtObject* parent) :
    Node(modelName, parent),
    m_inPorts("dynamicInPorts", "In Ports"),
    m_outPorts("dynamicOutPorts", "Out Ports"),
    m_option(option)
{
    if (m_option != NoDynamicPorts)
    {

        auto makeReadOnly = [](auto func){
            return [func = std::move(func)](QString const& id){
                GtAbstractProperty* tmp = func(id);
                tmp->setReadOnly(true);
                return tmp;
            };
        };

        QStringList inputTypes = inputWhiteList.empty() ?
                                     NodeDataFactory::instance().registeredTypeIds() :
                                     std::move(inputWhiteList);
        QStringList outputTypes = outputWhiteList.empty() ?
                                     NodeDataFactory::instance().registeredTypeIds() :
                                     std::move(outputWhiteList);

        GtPropertyStructDefinition portInfoIn{S_PORT_INFO_IN};
        portInfoIn.defineMember(S_PORT_TYPE, makeStringSelectionProperty(std::move(inputTypes)));
        portInfoIn.defineMember(S_PORT_CAPTION, gt::makeStringProperty());
        portInfoIn.defineMember(S_PORT_CAPTION_VISIBLE, gt::makeBoolProperty(true));
        portInfoIn.defineMember(S_PORT_OPTIONAL, gt::makeBoolProperty(true));
        portInfoIn.defineMember(S_PORT_ID, makeReadOnly(makeUIntProperty(invalid<PortId>())));

        GtPropertyStructDefinition portInfoOut{S_PORT_INFO_OUT};
        portInfoOut.defineMember(S_PORT_TYPE, makeStringSelectionProperty(std::move(outputTypes)));
        portInfoOut.defineMember(S_PORT_CAPTION, gt::makeStringProperty());
        portInfoOut.defineMember(S_PORT_CAPTION_VISIBLE, gt::makeBoolProperty(true));
        portInfoOut.defineMember(S_PORT_OPTIONAL, gt::makeBoolProperty(true));
        portInfoOut.defineMember(S_PORT_ID, makeReadOnly(makeUIntProperty(invalid<PortId>())));

        m_inPorts.registerAllowedType(portInfoIn);
        m_outPorts.registerAllowedType(portInfoOut);

        if (m_option != DynamicOutputOnly) registerPropertyStructContainer(m_inPorts);
        if (m_option != DynamicInputOnly)  registerPropertyStructContainer(m_outPorts);
    }

    connect(this, &Node::portAboutToBeDeleted,
            this, &DynamicNode::onPortDeleted,
            Qt::UniqueConnection);

    for (auto* ports : { &m_inPorts, &m_outPorts })
    {
        connect(ports, &GtPropertyStructContainer::entryAdded,
                this, &DynamicNode::onPortEntryAdded,
                Qt::UniqueConnection);
        connect(ports, &GtPropertyStructContainer::entryChanged,
                this, &DynamicNode::onPortEntryChanged,
                Qt::UniqueConnection);
        connect(ports, &GtPropertyStructContainer::entryRemoved,
                this, &DynamicNode::onPortEntryRemoved,
                Qt::UniqueConnection);
    }
}

DynamicNode::Option
DynamicNode::dynamicNodeOption() const
{
    return m_option;
}

int DynamicNode::offset(PortType type) const
{
    auto const& dynamicPorts = this->dynamicPorts(type);
    auto const& allPorts     = this->ports(type);
    int offset = allPorts.size() - dynamicPorts.size();
    return offset;
}

bool
DynamicNode::isDynamicPort(PortType type, PortIndex idx) const
{
    return static_cast<int>(idx) >= offset(type);
}

PortId
DynamicNode::addStaticInPort(PortInfo port, PortPolicy policy)
{
    port.optional = policy & PortPolicy::Optional;
    return insertPort(StaticPort, PortType::In, std::move(port));
}

PortId
DynamicNode::addStaticOutPort(PortInfo port)
{
    return insertPort(StaticPort, PortType::Out, std::move(port));
}

PortId
DynamicNode::addInPort(PortInfo port, PortPolicy policy)
{
    return insertInPort(std::move(port), -1, policy);
}

PortId
DynamicNode::addOutPort(PortInfo port)
{
    return insertOutPort(std::move(port), -1);
}

PortId
DynamicNode::insertInPort(PortInfo port, int idx, PortPolicy policy)
{
    port.optional = policy & PortPolicy::Optional;
    return insertPort(DynamicPort, PortType::In, std::move(port), idx);
}

PortId
DynamicNode::insertOutPort(PortInfo port, int idx)
{
    return insertPort(DynamicPort, PortType::Out, std::move(port), idx);
}

PortId
DynamicNode::insertPort(PortOption option, PortType type, PortInfo port, int idx)
{
    if (idx < 0) idx = std::numeric_limits<int>::max();

    auto& allPorts     = this->ports(type);
    auto& dynamicPorts = this->dynamicPorts(type);

    if (option == StaticPort)
    {
        return Node::insertPort(
            type, std::move(port), gt::clamp(idx, 0, (int)offset(type))
        );
    }

    // ignore changed signals of property container
    auto ignoreAdded = ignoreSignal(
        &dynamicPorts, &GtPropertyStructContainer::entryAdded,
        this, &DynamicNode::onPortEntryAdded
    );
    auto ignoreChanged = ignoreSignal(
        &dynamicPorts, &GtPropertyStructContainer::entryChanged,
        this, &DynamicNode::onPortEntryChanged
    );
    Q_UNUSED(ignoreAdded);
    Q_UNUSED(ignoreChanged);

    int offset = this->offset(type);

    int actualPortIdx = gt::clamp(idx, offset, (int)allPorts.size());

    PortId portId = Node::insertPort(type, port, actualPortIdx);

    int dynamicPortIdx = gt::clamp(idx, 0, (int)dynamicPorts.size());

    auto& entry = dynamicPorts.newEntry(type == PortType::In ? S_PORT_INFO_IN : S_PORT_INFO_OUT,
                                        std::next(dynamicPorts.begin(), dynamicPortIdx),
                                        QString::number(portId));
    entry.setMemberVal(S_PORT_ID, portId.value());
    entry.setMemberVal(S_PORT_TYPE, port.typeId);
    entry.setMemberVal(S_PORT_CAPTION, port.caption);
    entry.setMemberVal(S_PORT_CAPTION_VISIBLE, port.captionVisible);
    entry.setMemberVal(S_PORT_OPTIONAL, port.optional);

    // update port typeId to a valid value (due to whitelists)
    if (auto* p = this->port(portId))
    {
        p->typeId = entry.getMemberVal<QString>(S_PORT_TYPE);
        emit portChanged(portId);
    }

    return portId;
}

void
DynamicNode::onPortDeleted(PortType type, PortIndex idx)
{
    PortId portId = this->portId(type, idx);
    if (portId == invalid<PortId>())
    {
        gtWarning() << tr("Removing dynamic port failed! (Port '%1' not found, type: %2)")
                           .arg(portId).arg(toString(type));
        return;
    }

    auto& ports = dynamicPorts(type);

    // ignore removed signal of property container
    auto ignoreRemoved = ignoreSignal(
        &ports, &GtPropertyStructContainer::entryRemoved,
        this, &DynamicNode::onPortEntryRemoved
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
DynamicNode::onPortEntryAdded(int idx)
{
    auto const makeError = [](){
        return tr("Adding dynamic port entry failed!");
    };

    auto* dynamicPorts = toDynamicPorts(sender());
    auto* entry = propertyAt(dynamicPorts, idx);
    if (!entry) return;

    PortType type = toPortType(*dynamicPorts);

    // get port id from entry ident
    bool ok = true;
    auto ident = entry->ident().toUInt(&ok);

    PortId portId;
    if (ok) portId = PortId(ident);

    // check if port id already exists (entry probably added in constructor)
    if (ok && port(portId))
    {
        gtWarning() << makeError()
                    << tr("(Port '%1' was already added)").arg(portId);
        return;
    }

    QString typeId  = entry->template getMemberVal<QString>(S_PORT_TYPE);
    QString caption = entry->template getMemberVal<QString>(S_PORT_CAPTION);
    bool captionVisible = entry->template getMemberVal<bool>(S_PORT_CAPTION_VISIBLE);
    bool optional = entry->template getMemberVal<bool>(S_PORT_OPTIONAL);

    bool updatePortId = true;
    // check if port id saved is valid and use that then
    {
        auto tmpPortId = PortId(entry->template getMemberVal<unsigned>(S_PORT_ID));
        if (tmpPortId != invalid<PortId>())
        {
            portId = tmpPortId;
            updatePortId = false;
        }
    }

    if (auto* p = port(portId))
    {
        gtInfo().verbose()
            << makeError()
            << tr("(Port already exists: %1)").arg(toString(*p));
        return;
    }

    auto portInfo = PortInfo::customId(portId, typeId, caption, captionVisible, optional);

    idx += offset(type) + 1;

    portId = Node::insertPort(type, portInfo, idx);
    if (portId == invalid<PortId>())
    {
        gtWarning() << makeError()
                    << tr("(Failed to insert dynamic port!)");
        dynamicPorts->removeEntry(std::next(dynamicPorts->begin(), idx));
        return;
    }

    if (updatePortId)
    {
        entry->setMemberVal(S_PORT_ID, QVariant::fromValue(portId.value()));
    }

    emit portChanged(portId);
}

void
DynamicNode::onPortEntryChanged(int idx, GtAbstractProperty* p)
{
    auto* dynamicPorts = toDynamicPorts(sender());
    if (!dynamicPorts) return;

    PortType type = toPortType(*dynamicPorts);

    auto* entry = propertyAt(dynamicPorts, idx);
    if (!entry) return;

    idx += offset(type);

    PortId portId = this->portId(type, PortIndex::fromValue(idx));

    auto* port = this->port(portId);
    if (!port)
    {
        gtWarning() << tr("Updating dynamic port entry failed! (Port idx '%1' not found)")
                           .arg(idx);
        return;
    }

    QString typeId  = entry->template getMemberVal<QString>(S_PORT_TYPE);
    QString caption = entry->template getMemberVal<QString>(S_PORT_CAPTION);
    bool captionVisible = entry->template getMemberVal<bool>(S_PORT_CAPTION_VISIBLE);
    bool optional = entry->template getMemberVal<bool>(S_PORT_OPTIONAL);
    PortId newPortId(entry->template getMemberVal<unsigned>(S_PORT_ID));

    port->typeId = std::move(typeId);
    port->caption = std::move(caption);
    port->captionVisible = captionVisible;
    port->optional = optional;

    if (portId != newPortId && newPortId != invalid<PortId>())
    {
        auto portInfo = PortInfo::customId(newPortId, *port);

        // hacky solution -> remove port and insert new port with new id
        auto ignoreRemoved = ignoreSignal(
            this, &Node::portAboutToBeDeleted,
            this, &DynamicNode::onPortDeleted
        );
        Q_UNUSED(ignoreRemoved);

        removePort(portId);
        insertPort(type, std::move(portInfo), idx);
        return;
    }

    emit portChanged(port->id());
}

void
DynamicNode::onPortEntryRemoved(int idx)
{
    auto* dynamicPorts = toDynamicPorts(sender());
    if (!dynamicPorts) return;

    PortType type = toPortType(*dynamicPorts);

    idx += offset(type) - 1;

    PortId portId = this->portId(type, PortIndex::fromValue(idx));

    if (portId == invalid<PortId>())
    {
        gtWarning() << tr("Removing dynamic port entry failed! (Port idx '%1' not found)")
                           .arg(idx);
        return;
    }

    // ignore removed signal
    auto ignoreRemoved = ignoreSignal(
        this, &Node::portAboutToBeDeleted,
        this, &DynamicNode::onPortDeleted
    );
    Q_UNUSED(ignoreRemoved);

    removePort(portId);
}

PortType
DynamicNode::toPortType(GtPropertyStructContainer const& container) const
{
    if (&container == &m_inPorts) return PortType::In;
    if (&container == &m_outPorts) return PortType::Out;
    return PortType::NoType;
}

GtPropertyStructContainer&
DynamicNode::dynamicPorts(PortType type) noexcept(false)
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

GtPropertyStructContainer const&
DynamicNode::dynamicPorts(PortType type) const noexcept(false)
{
    return const_cast<DynamicNode*>(this)->dynamicPorts(type);
}

GtPropertyStructContainer*
DynamicNode::toDynamicPorts(QObject* obj)
{
    if (obj == &m_inPorts)  return &m_inPorts;
    if (obj == &m_outPorts) return &m_outPorts;
    return nullptr;
}

GtPropertyStructInstance*
DynamicNode::propertyAt(GtPropertyStructContainer* container, int idx)
{
    if (!container) return nullptr;

    try
    {
        return &container->at(idx);
    }
    catch (std::out_of_range const& e)
    {
        gtError().nospace() << __FUNCTION__ << ": " << e.what();
    }
    return nullptr;
}