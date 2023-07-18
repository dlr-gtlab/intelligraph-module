/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphobjectmodel.h"

#include "gt_igjsonadpater.h"
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

GtIntelliGraphObjectModel::GtIntelliGraphObjectModel(GtIntelliGraphNode& node)
{
    init(node);
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
    m_node->setActive();

    connect(this, &GtIntelliGraphObjectModel::dataUpdated,
            m_node, [=](unsigned idx){
        if (sender() != m_node) emit m_node->outDataUpdated(gt::ig::PortIndex{idx});
    });
    connect(m_node, &GtIntelliGraphNode::outDataUpdated,
            this, [=](gt::ig::PortIndex idx){
        if (sender() != this) emit this->dataUpdated(idx);
    });

    connect(this, &GtIntelliGraphObjectModel::dataInvalidated,
            m_node, [=](unsigned idx){
        if (sender() != m_node) emit m_node->outDataInvalidated(gt::ig::PortIndex{idx});
    });
    connect(m_node, &GtIntelliGraphNode::outDataInvalidated,
            this, [=](gt::ig::PortIndex idx){
        if (sender() != this) emit this->dataInvalidated(idx);
    });

    connect(m_node, &GtIntelliGraphNode::portAboutToBeDeleted,
            this, [=](gt::ig::PortType type, gt::ig::PortIndex first){
        emit portsAboutToBeDeleted(cast_port_type(type), first, first);
    });
    connect(m_node, &GtIntelliGraphNode::portDeleted,
            this, &GtIntelliGraphObjectModel::portsDeleted);

    connect(m_node, &GtIntelliGraphNode::portAboutToBeInserted,
            this, [=](gt::ig::PortType type, gt::ig::PortIndex first){
        emit portsAboutToBeInserted(cast_port_type(type), first, first);
    });
    connect(m_node, &GtIntelliGraphNode::portInserted,
            this, &GtIntelliGraphObjectModel::portsInserted);

//    GTIG_SETUP_SIGNALS(dataUpdated, outDataUpdated);
//    GTIG_SETUP_SIGNALS(dataInvalidated, outDataInvalidated);
//    GTIG_SETUP_SIGNALS(computingStarted);
//    GTIG_SETUP_SIGNALS(computingFinished);
//    GTIG_SETUP_SIGNALS(embeddedWidgetSizeUpdated);

//    connect(this, &GtIntelliGraphObjectModel::nodeInitialized,
//            m_node, &GtIntelliGraphNode::updateNode);

    gtDebug().verbose() << "INITIALIZED:" << m_node->objectName();

    emit nodeInitialized();
}

GtIntelliGraphObjectModel::QtNodeFlags
GtIntelliGraphObjectModel::flags() const
{
    if (!m_node) return QtNodes::NodeDelegateModel::flags();

    QtNodeFlags flags{};

    if (m_node->nodeFlags() & NodeFlag::Resizable)
    {
        flags |= QtNodeFlag::Resizable;
    }

    if (m_node->nodeFlags() & NodeFlag::Unique)
    {
        flags |= QtNodeFlag::Unique;
    }

    if (m_node->objectFlags() & GtObject::UserDeletable)
    {
        flags |= QtNodeFlag::Deletable;
    }

    return flags;
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
GtIntelliGraphObjectModel::nPorts(const QtPortType type) const
{
    if (!m_node) return 0;

    return m_node->ports(cast_port_type(type)).size();
}

GtIntelliGraphObjectModel::QtNodeDataType
GtIntelliGraphObjectModel::dataType(const QtPortType type, const QtPortIndex idx) const
{
    if (!m_node) return {};

    auto const& data = m_node->ports(cast_port_type(type));

    if (idx >= data.size()) return {};

    QString const& typeId = data.at(idx).typeId;

    QString typeName = GtIntelliGraphDataFactory::instance().typeName(typeId);

    if (typeName.isEmpty())
    {
        return QtNodeDataType{
            QStringLiteral("__unkown__"),
            QStringLiteral("<unkown>")
        };
    }

    return QtNodeDataType{typeId, typeName};
}

bool
GtIntelliGraphObjectModel::portCaptionVisible(QtPortType type, QtPortIndex idx) const
{
    if (!m_node) return false;

    auto const& data = m_node->ports(cast_port_type(type));
    if (idx >= data.size()) return false;

    return data.at(idx).captionVisible;
}

QString
GtIntelliGraphObjectModel::portCaption(QtPortType type, QtPortIndex idx) const
{
    if (!m_node) return {};

    auto const& data = m_node->ports(cast_port_type(type));
    if (idx >= data.size()) return {};

    return data.at(idx).caption;
}

GtIntelliGraphObjectModel::QtNodeData
GtIntelliGraphObjectModel::outData(const QtPortIndex port)
{
    if (!m_node) return {};

    auto data = m_node->outData(gt::ig::PortIndex{port});
    return std::make_shared<GtIgObjectModelData>(std::move(data));
}

void
GtIntelliGraphObjectModel::setInData(QtNodeData nodeData, const QtPortIndex port)
{
    if (!m_node) return;

    if (auto data = std::dynamic_pointer_cast<GtIgObjectModelData>(nodeData))
    {
        m_node->setInData(gt::ig::PortIndex{port}, data->data());
        return;
    }

    m_node->setInData(gt::ig::PortIndex{port}, nullptr);
}

QWidget*
GtIntelliGraphObjectModel::embeddedWidget()
{
    return m_node ? m_node->embeddedWidget() : nullptr;
}

QJsonObject
GtIntelliGraphObjectModel::save() const
{
    if (m_node)
    {
        return gt::ig::toJson(*m_node)["internal-data"].toObject();
    }

    return QtNodes::NodeDelegateModel::save();
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

    gt::ig::mergeFromJson(json, *m_node);

    gtDebug().verbose() << "NODE LOADED:" << m_node->objectName();

    m_node->updateNode();
}

void
GtIntelliGraphObjectModel::outputConnectionCreated(const QtConnectionId&)
{
    if (!m_node) return;
    
    auto const& ports = m_node->ports(cast_port_type(QtPortType::In));

    if (ports.empty()) m_node->updateNode();
}

void
GtIntelliGraphObjectModel::outputConnectionDeleted(const QtConnectionId&)
{
    if (!m_node) return;

//    auto const& ports = m_node->ports(cast_port_type(PortType::In));

//    if (ports.size() == 0) m_node->updateNode();
}
