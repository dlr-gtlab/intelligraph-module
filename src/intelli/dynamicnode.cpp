/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/dynamicnode.h"
#include "intelli/nodedatafactory.h"
#include "intelli/utilities.h"
#include "intelli/property/stringselection.h"
#include "intelli/property/uint.h"
#include "intelli/private/utils.h"

#include "gt_propertystructcontainer.h"
#include "gt_structproperty.h"
#include "gt_stringproperty.h"
#include "gt_boolproperty.h"
#include "gt_exceptions.h"
#include "gt_version.h"

using namespace intelli;

QString const S_PORT_INFO_IN  = QStringLiteral("PortInfoIn");
QString const S_PORT_INFO_OUT = QStringLiteral("PortInfoOut");

QString const S_PORT_TYPE = QStringLiteral("TypeId");
QString const S_PORT_CAPTION = QStringLiteral("Caption");
QString const S_PORT_CAPTION_VISIBLE = QStringLiteral("CaptionVisible");
QString const S_PORT_OPTIONAL = QStringLiteral("Optional");
QString const S_PORT_ID = QStringLiteral("PortId");

struct DynamicNode::Impl
{
    explicit Impl(size_t opt) : option(opt) {};

    /// property container for the in ports
    GtPropertyStructContainer inPorts{"dynamicInPorts", "In Ports"};
    /// property container for the out ports
    GtPropertyStructContainer outPorts{"dynamicOutPorts", "Out Ports"};

    /// Node option
    size_t option = DynamicInputAndOutput;
};

DynamicNode::DynamicNode(QString const& modelName,
                         size_t option,
                         GtObject* parent) :
    DynamicNode(modelName, QStringList{}, QStringList{}, option, parent)
{ }

DynamicNode::DynamicNode(QString const& modelName,
                         QStringList inputWhiteList,
                         QStringList outputWhiteList,
                         size_t option,
                         GtObject* parent) :
    Node(modelName, parent),
    pimpl(std::make_unique<Impl>(option))
{
    if (pimpl->option != NoDynamicPorts)
    {
        auto makeReadOnly = [](auto func){
            return [func = std::move(func)](QString const& id){
                GtAbstractProperty* tmp = func(id);
                tmp->setReadOnly(true);
                return tmp;
            };
        };

        QStringList inputTypes = inputWhiteList.empty() ?
                                     NodeDataFactory::instance().validTypeIds() :
                                     std::move(inputWhiteList);
        QStringList outputTypes = outputWhiteList.empty() ?
                                     NodeDataFactory::instance().validTypeIds() :
                                     std::move(outputWhiteList);

        inputTypes.sort();
        outputTypes.sort();
      
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

        pimpl->inPorts.registerAllowedType(portInfoIn);
        pimpl->outPorts.registerAllowedType(portInfoOut);

        if (pimpl->option & DynamicInput  || pimpl->option & NoUserDynamicInput)
        {
#if GT_VERSION >= GT_VERSION_CHECK(2, 1, 0)
            if (pimpl->option & NoUserDynamicInput)
            {
                pimpl->inPorts.setFlags(GtPropertyStructContainer::Hidden);
            }
#endif
            registerPropertyStructContainer(pimpl->inPorts);
        }
        if (pimpl->option & DynamicOutput || pimpl->option & NoUserDynamicOutput)
        {
#if GT_VERSION >= GT_VERSION_CHECK(2, 1, 0)
            if (pimpl->option & NoUserDynamicOutput)
            {
                pimpl->outPorts.setFlags(GtPropertyStructContainer::Hidden);
            }
#endif
            registerPropertyStructContainer(pimpl->outPorts);
        }
    }

    connect(this, &Node::portAboutToBeDeleted,
            this, &DynamicNode::onPortDeleted,
            Qt::UniqueConnection);

    for (auto* ports : { &pimpl->inPorts, &pimpl->outPorts })
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

DynamicNode::~DynamicNode() = default;

size_t
DynamicNode::dynamicNodeOption() const
{
    return pimpl->option;
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
    auto ignoreAdded = utils::ignoreSignal(
        &dynamicPorts, &GtPropertyStructContainer::entryAdded,
        this, &DynamicNode::onPortEntryAdded
    );
    auto ignoreChanged = utils::ignoreSignal(
        &dynamicPorts, &GtPropertyStructContainer::entryChanged,
        this, &DynamicNode::onPortEntryChanged
    );
    Q_UNUSED(ignoreAdded);
    Q_UNUSED(ignoreChanged);

    int offset = this->offset(type);

    int actualPortIdx = gt::clamp(idx, offset, (int)allPorts.size());

    PortId portId = Node::insertPort(type, port, actualPortIdx);
    if (!portId.isValid()) return portId;

    int dynamicPortIdx = gt::clamp(idx, 0, (int)dynamicPorts.size());

    GtPropertyStructInstance& entry =
        dynamicPorts.newEntry(type == PortType::In ? S_PORT_INFO_IN : S_PORT_INFO_OUT,
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

Node::PortId
DynamicNode::verifyPortId(PortId portId)
{
    // nothing to do here
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

    GtPropertyStructContainer& dynamicPorts = this->dynamicPorts(type);

    // ignore removed signal of property container
    auto ignoreRemoved = utils::ignoreSignal(
        &dynamicPorts, &GtPropertyStructContainer::entryRemoved,
        this, &DynamicNode::onPortEntryRemoved
    );
    Q_UNUSED(ignoreRemoved);

    auto iter = std::find_if(dynamicPorts.begin(), dynamicPorts.end(),
                             [=](auto const& iter){
        bool ok = true;
        auto id = PortId::fromValue(iter.template getMemberVal<int>(S_PORT_ID, &ok));
        return ok && id == portId;
    });

    if (iter == dynamicPorts.end()) return;

    gtInfo().verbose() << tr("Removing dynamic port entry:") << portId;

    dynamicPorts.removeEntry(iter);
}

void
DynamicNode::onPortEntryAdded(int idx)
{
    auto const makeError = [](){
        return tr("Adding dynamic port entry failed!");
    };

    GtPropertyStructContainer* dynamicPorts = toDynamicPorts(sender());
    GtPropertyStructInstance* entry = propertyAt(dynamicPorts, idx);
    if (!entry) return;

    PortType type = toPortType(*dynamicPorts);

    // get port id from entry ident
    PortId portId{};

    bool ok = true;
    unsigned rawPortId = entry->ident().toUInt(&ok);
    if (ok) portId = PortId::fromValue(rawPortId);

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
    // (e.g. if restored using memento)
    {
        auto tmpPortId = PortId(entry->template getMemberVal<unsigned>(S_PORT_ID));
        if (tmpPortId != invalid<PortId>())
        {
            portId = tmpPortId;
            updatePortId = false;
        }
    }

    portId = verifyPortId(portId);

    if (auto* p = port(portId))
    {
        gtInfo().verbose()
            << makeError()
            << tr("(Port already exists: %1)").arg(toString(*p));
        return;
    }

    PortInfo port = PortInfo::customId(portId, typeId, caption, captionVisible, optional);

    idx += offset(type) + 1;

    portId = Node::insertPort(type, port, idx);
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
    GtPropertyStructContainer* dynamicPorts = toDynamicPorts(sender());
    if (!dynamicPorts) return;

    PortType type = toPortType(*dynamicPorts);

    GtPropertyStructInstance* entry = propertyAt(dynamicPorts, idx);
    if (!entry) return;

    idx += offset(type);

    PortId portId  = this->portId(type, PortIndex::fromValue(idx));
    PortInfo* port = this->port(portId);
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
    PortId newPortId{entry->template getMemberVal<unsigned>(S_PORT_ID)};

    port->typeId = std::move(typeId);
    port->caption = std::move(caption);
    port->captionVisible = captionVisible;
    port->optional = optional;

    if (portId != newPortId && newPortId != invalid<PortId>())
    {
        PortInfo updatedPort = PortInfo::customId(newPortId, *port);

        // hacky solution -> remove port and insert new port with new id
        auto ignoreRemoved = utils::ignoreSignal(
            this, &Node::portAboutToBeDeleted,
            this, &DynamicNode::onPortDeleted
        );
        Q_UNUSED(ignoreRemoved);

        removePort(portId);

        insertPort(type, std::move(updatedPort), idx);
        return;
    }

    emit portChanged(port->id());
}

void
DynamicNode::onPortEntryRemoved(int idx)
{
    GtPropertyStructContainer* dynamicPorts = toDynamicPorts(sender());
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
    auto ignoreRemoved = utils::ignoreSignal(
        this, &Node::portAboutToBeDeleted,
        this, &DynamicNode::onPortDeleted
    );
    Q_UNUSED(ignoreRemoved);

    removePort(portId);
}

PortType
DynamicNode::toPortType(GtPropertyStructContainer const& container) const
{
    if (&container == &pimpl->inPorts) return PortType::In;
    if (&container == &pimpl->outPorts) return PortType::Out;
    return PortType::NoType;
}

GtPropertyStructContainer&
DynamicNode::dynamicPorts(PortType type) noexcept(false)
{
    switch (type)
    {
    case PortType::In:
        return pimpl->inPorts;
    case PortType::Out:
        return pimpl->outPorts;
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
    if (obj == &pimpl->inPorts)  return &pimpl->inPorts;
    if (obj == &pimpl->outPorts) return &pimpl->outPorts;
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
