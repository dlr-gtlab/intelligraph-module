/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphobjectmodel.h"

#include "gt_intelligraphnodefactory.h"
#include "gt_intelligraphdatafactory.h"

/// macro to setup singals in such a way that they will be forwarded but wont
/// start an infinite loop
#define GTIG_SETUP_SIGNALS(SIG_A, SIG_B) \
    connect(this, &GtIntelliGraphObjectModel::SIG_A, \
            m_node, [=](auto&& ...args){ \
                    if (sender() != m_node) m_node->SIG_B(args...); \
            }); \
    connect(m_node, &GtIntelliGraphNode::SIG_B, \
            this, [=](auto&& ...args){ \
                    if (sender() != this) this->SIG_A(args...);\
            });

GtIntelliGraphObjectModel::GtIntelliGraphObjectModel(const QString& className)
{
    auto& factory = GtIntelliGraphNodeFactory::instance();

    auto node = factory.newNode(className);

    node->setParent(this);
    init(*node.release());
}

void
GtIntelliGraphObjectModel::init(GtIntelliGraphNode& node)
{
    if (m_node)
    {
        // dunno if this is necessary
        this->disconnect(m_node);
        m_node->disconnect(this);

        // we dont want to carry dead weight
        if (m_node->parent() == this) m_node->deleteLater();
    }

    m_node = &node;
    
    GTIG_SETUP_SIGNALS(dataUpdated, outDataUpdated);
    GTIG_SETUP_SIGNALS(dataInvalidated, outDataInvalidated);
//    GTIG_SETUP_SIGNALS(computingStarted);
//    GTIG_SETUP_SIGNALS(computingFinished);
//    GTIG_SETUP_SIGNALS(embeddedWidgetSizeUpdated);
//    GTIG_SETUP_SIGNALS(portsAboutToBeDeleted);
//    GTIG_SETUP_SIGNALS(portsDeleted);
//    GTIG_SETUP_SIGNALS(portsAboutToBeInserted);
//    GTIG_SETUP_SIGNALS(portsInserted);

//    connect(this, &GtIntelliGraphObjectModel::nodeInitialized,
//            m_node, &GtIntelliGraphNode::updateNode);

    gtDebug().verbose() << "INITIALIZED:" << m_node->objectName();

    emit nodeInitialized();
}

bool
GtIntelliGraphObjectModel::resizable() const
{
    return m_node ? m_node->nodeFlags() & NodeFlag::Resizable : false;
}

bool
GtIntelliGraphObjectModel::captionVisible() const
{
    return m_node ? !(m_node->nodeFlags() & NodeFlag::HideCaption) : false;
}

QString
GtIntelliGraphObjectModel::caption() const
{
    return m_node ? m_node->caption() : QString{};
}

QString
GtIntelliGraphObjectModel::name() const
{
    return m_node ? m_node->modelName() : QStringLiteral("<invalid_node>");
}

unsigned int
GtIntelliGraphObjectModel::nPorts(const PortType type) const
{
    if (!m_node) return 0;

    return m_node->ports(cast_port_type(type)).size();
}

GtIntelliGraphObjectModel::NodeDataType
GtIntelliGraphObjectModel::dataType(const PortType type, const PortIndex idx) const
{
    if (!m_node) return {};

    auto const& data = m_node->ports(cast_port_type(type));

    if (idx >= data.size()) return {};

    QString const& typeId = data.at(idx).typeId;

    QString typeName = GtIntelliGraphDataFactory::instance().typeName(typeId);

    if (typeName.isEmpty())
    {
        return NodeDataType{
            QStringLiteral("__unkown__"),
            QStringLiteral("<unkown>")
        };
    }

    return NodeDataType{typeId, typeName};
}

bool
GtIntelliGraphObjectModel::portCaptionVisible(PortType type, PortIndex idx) const
{
    // this method indicates whether the custom port caption should be visible
    // therefore we have to return false by default

    if (!m_node) return false;

    auto const& data = m_node->ports(cast_port_type(type));
    if (idx >= data.size()) return false;

    return true;
}

QString
GtIntelliGraphObjectModel::portCaption(PortType type, PortIndex idx) const
{
    // returning an empty string will show the defualt port caption

    if (!m_node) return {};

    auto const& data = m_node->ports(cast_port_type(type));
    if (idx >= data.size()) return {};

    auto& port = data.at(idx);

    QString typeName = GtIntelliGraphDataFactory::instance().typeName(port.typeId);

    if (port.captionVisible)
    {
        return port.caption.isEmpty() ?
                   typeName :
                   QStringLiteral("%1 (%2)").arg(port.caption, typeName);
    }

    return {};
}

GtIntelliGraphObjectModel::NodeData
GtIntelliGraphObjectModel::outData(const PortIndex port)
{
    if (!m_node) return {};

    auto data = m_node->outData(port);
    return std::make_shared<GtIgObjectModelData>(std::move(data));
}

void
GtIntelliGraphObjectModel::setInData(NodeData nodeData, const PortIndex port)
{
    if (!m_node) return;

    if (auto data = std::dynamic_pointer_cast<GtIgObjectModelData>(nodeData))
    {
        return m_node->setInData(port, data->data());
    }

    return m_node->setInData(port, nullptr);
}

QWidget*
GtIntelliGraphObjectModel::embeddedWidget()
{
    return m_node ? m_node->embeddedWidget() : nullptr;
}

QJsonObject
GtIntelliGraphObjectModel::save() const
{
    auto json = QtNodes::NodeDelegateModel::save();

    if (m_node)
    {
        m_node->toJsonMemento(json);
    }

    return json;
}

void
GtIntelliGraphObjectModel::load(const QJsonObject& json)
{
    if (!m_node) return;

    auto modelName = json["model-name"].toString();

    if (modelName != name())
    {
        gtError() << tr("Failed to load model data from json! "
                        "Invalid modelname '%1', was expecting '%2'!")
                     .arg(modelName, name());
        return;
    }

    m_node->mergeJsonMemento(json);

    gtDebug().verbose() << "NODE LOADED:" << m_node->objectName();

    m_node->updateNode();
}

void
GtIntelliGraphObjectModel::outputConnectionCreated(const ConnectionId&)
{
    if (!m_node) return;

    auto const& ports = m_node->ports(cast_port_type(PortType::In));

    if (ports.empty()) m_node->updateNode();
}

void
GtIntelliGraphObjectModel::outputConnectionDeleted(const ConnectionId&)
{
    if (!m_node) return;

//    auto const& ports = m_node->ports(cast_port_type(PortType::In));

//    if (ports.size() == 0) m_node->updateNode();
}
