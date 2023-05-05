/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphobjectmodel.h"

#include "gt_intelligraph.h"

/// macro to setup singals in such a way that they will be forwarded but wont
/// start an infinite loop
#define GTIG_SETUP_SIGNALS(SIG) \
    connect(this, &GtIntelliGraphObjectModel::SIG, \
            m_node, [=](auto&& ...args){ \
                    if (sender() != m_node) m_node->SIG(args...); \
            }); \
    connect(m_node, &GtIntelliGraphNode::SIG, \
            this, [=](auto&& ...args){ \
                    if (sender() != this) this->SIG(args...);\
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

    GTIG_SETUP_SIGNALS(dataUpdated);
    GTIG_SETUP_SIGNALS(dataInvalidated);
    GTIG_SETUP_SIGNALS(computingStarted);
    GTIG_SETUP_SIGNALS(computingFinished);
    GTIG_SETUP_SIGNALS(embeddedWidgetSizeUpdated);
    GTIG_SETUP_SIGNALS(portsAboutToBeDeleted);
    GTIG_SETUP_SIGNALS(portsDeleted);
    GTIG_SETUP_SIGNALS(portsAboutToBeInserted);
    GTIG_SETUP_SIGNALS(portsInserted);

    connect(this, &GtIntelliGraphObjectModel::nodeInitialized,
            m_node, &GtIntelliGraphNode::updateNode);

    gtDebug().verbose() << "INITIALIZED" << m_node->objectName();
    emit nodeInitialized();
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

    gtDebug() << "Object loaded!" << m_node->objectName();

    m_node->updateNode();
}
