/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/adapter/objectmodel.h"

#include "intelli/adapter/jsonadapter.h"
#include "intelli/nodefactory.h"
#include "intelli/nodedatafactory.h"

using namespace intelli;

ObjectModel::ObjectModel(const QString& className)
{
    auto& factory = NodeFactory::instance();

    auto node = factory.newNode(className);

    node->setParent(this);
    init(*node.release());
}

ObjectModel::ObjectModel(Node& node)
{
    init(node);
}

void
ObjectModel::init(Node& node)
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

    connect(this, &ObjectModel::dataUpdated,
            m_node, [=](unsigned idx){
        if (sender() != m_node) emit m_node->outDataUpdated(PortIndex{idx});
    });
    connect(m_node, &Node::outDataUpdated,
            this, [=](PortIndex idx){
        if (sender() != this) emit this->dataUpdated(idx);
    });

    connect(this, &ObjectModel::dataInvalidated,
            m_node, [=](unsigned idx){
        if (sender() != m_node) emit m_node->outDataInvalidated(PortIndex{idx});
    });
    connect(m_node, &Node::outDataInvalidated,
            this, [=](PortIndex idx){
        if (sender() != this) emit this->dataInvalidated(idx);
    });
    
    connect(m_node, &Node::portAboutToBeDeleted,
            this, [=](PortType type, PortIndex first){
        emit portsAboutToBeDeleted(cast_port_type(type), first, first);
    });
    connect(m_node, &Node::portDeleted,
            this, &ObjectModel::portsDeleted);
    
    connect(m_node, &Node::portAboutToBeInserted,
            this, [=](PortType type, PortIndex first){
        emit portsAboutToBeInserted(cast_port_type(type), first, first);
    });
    connect(m_node, &Node::portInserted,
            this, &ObjectModel::portsInserted);
    
    connect(m_node, &Node::computingStarted, this, [this](){
        m_evaluating = true;
    });
    connect(m_node, &Node::computingFinished, this, [this](){
        m_evaluating = false;
    });

    gtDebug().verbose() << "INITIALIZED:" << m_node->objectName();

    emit nodeInitialized();
}

ObjectModel::QtNodeFlags
ObjectModel::flags() const
{
    if (!m_node) return QtNodes::NodeDelegateModel::flags();

    QtNodeFlags flags{};

    auto nodeFlags = m_node->nodeFlags();

    if (nodeFlags & NodeFlag::Resizable)
    {
        flags |= QtNodeFlag::Resizable;
    }

    if (nodeFlags & NodeFlag::Unique)
    {
        flags |= QtNodeFlag::Unique;
    }

    if (m_node->objectFlags() & GtObject::UserDeletable)
    {
        flags |= QtNodeFlag::Deletable;
    }

    m_evaluating ? flags |=  QtNodeFlag::Evaluating :
                   flags &= ~QtNodeFlag::Evaluating;

    return flags;
}

bool
ObjectModel::captionVisible() const
{
    return m_node ? !(m_node->nodeFlags() & NodeFlag::HideCaption) : false;
}

QString
ObjectModel::caption() const
{
    return m_node ? m_node->caption() : QString{};
}

QString
ObjectModel::name() const
{
    return m_node ? m_node->modelName() : QStringLiteral("<invalid_node>");
}

unsigned int
ObjectModel::nPorts(const QtPortType type) const
{
    if (!m_node) return 0;

    return m_node->ports(cast_port_type(type)).size();
}

ObjectModel::QtNodeDataType
ObjectModel::dataType(const QtPortType type, const QtPortIndex idx) const
{
    if (!m_node) return {};

    auto const& data = m_node->ports(cast_port_type(type));

    if (idx >= data.size()) return {};

    QString const& typeId = data.at(idx).typeId;
    
    QString typeName = NodeDataFactory::instance().typeName(typeId);

    if (typeName.isEmpty())
    {
        return QtNodeDataType{
            QStringLiteral("__unknown__"),
            QStringLiteral("<unknown>")
        };
    }

    return QtNodeDataType{typeId, typeName};
}

bool
ObjectModel::portCaptionVisible(QtPortType type, QtPortIndex idx) const
{
    if (!m_node) return false;

    auto const& data = m_node->ports(cast_port_type(type));
    if (idx >= data.size()) return false;

    return data.at(idx).captionVisible;
}

QString
ObjectModel::portCaption(QtPortType type, QtPortIndex idx) const
{
    if (!m_node) return {};

    auto const& data = m_node->ports(cast_port_type(type));
    if (idx >= data.size()) return {};

    return data.at(idx).caption;
}

ObjectModel::QtNodeData
ObjectModel::outData(const QtPortIndex port)
{
    if (!m_node) return {};

    auto data = m_node->outData(PortIndex{port});
    return std::make_shared<ObjectModelData>(std::move(data));
}

void
ObjectModel::setInData(QtNodeData nodeData, const QtPortIndex port)
{
    if (!m_node) return;

    if (auto data = std::dynamic_pointer_cast<ObjectModelData>(nodeData))
    {
        m_node->setInData(PortIndex{port}, data->data());
        return;
    }

    m_node->setInData(PortIndex{port}, nullptr);
}

QWidget*
ObjectModel::embeddedWidget()
{
    if (!m_node) return nullptr;

    return m_node->embeddedWidget();
}

QJsonObject
ObjectModel::save() const
{
    if (m_node)
    {
        return toJson(*m_node)["internal-data"].toObject();
    }

    return QtNodes::NodeDelegateModel::save();
}

void
ObjectModel::load(const QJsonObject& json)
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

    mergeFromJson(json, *m_node);

    gtDebug().verbose() << "NODE LOADED:" << m_node->objectName();

    m_node->updateNode();
}

void
ObjectModel::outputConnectionCreated(const QtConnectionId&) { }

void
ObjectModel::outputConnectionDeleted(const QtConnectionId&) { }
