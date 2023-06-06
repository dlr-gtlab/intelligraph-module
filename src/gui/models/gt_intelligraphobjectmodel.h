/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLIGRAPHOBJECTMODEL_H
#define GT_INTELLIGRAPHOBJECTMODEL_H

#include "gt_intelligraphnode.h"

#include <QtNodes/NodeDelegateModel>

class GtIntelliGraph;
class GtIntelliGraphObjectModel final : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:

    using NodeFlag         = GtIntelliGraphNode::NodeFlag;
    using NodeFlags        = GtIntelliGraphNode::NodeFlags;
    using PortType         = GtIntelliGraphNode::PortType;
    using PortIndex        = GtIntelliGraphNode::PortIndex;
    using NodeDataType     = GtIntelliGraphNode::NodeDataType;
    using NodeData         = GtIntelliGraphNode::NodeData;
    using ConnectionPolicy = GtIntelliGraphNode::ConnectionPolicy;
    using ConnectionId     = GtIntelliGraphNode::ConnectionId;

    explicit GtIntelliGraphObjectModel(QString const& className);

    /// initializes the model with a new node object
    void init(GtIntelliGraphNode& node);

    GtIntelliGraphNode& node() { assert(m_node); return *m_node; }
    GtIntelliGraphNode const& node() const { assert(m_node); return *m_node; }

    bool resizable() const override
    {
        return m_node ? m_node->nodeFlags() & NodeFlag::Resizable :
                        false;
    }

    bool captionVisible() const override
    {
        return m_node ? !(m_node->nodeFlags() & NodeFlag::CaptionInvisble) :
                        false;
    }

    QString caption() const override
    {
        return m_node ? m_node->caption() : QString{};
    }

    QString name() const override
    {
        return m_node ? m_node->modelName() : QStringLiteral("<invalid_node>");
    }

    unsigned nPorts(PortType const type) const override
    {
        return m_node ? m_node->nPorts(type) : 0;
    }

    NodeDataType dataType(PortType const type, PortIndex const idx) const override
    {
        return m_node ? m_node->dataType(type, idx) : NodeDataType{};
    }

    bool portCaptionVisible(PortType type, PortIndex idx) const override
    {
        return m_node ? m_node->portCaptionVisible(type, idx) : false;
    }

    QString portCaption(PortType type, PortIndex idx) const override
    {
        return m_node ? m_node->portCaption(type, idx) : QString{};
    }

    NodeData outData(PortIndex const port) override
    {
        return m_node ? m_node->outData(port) : NodeData{};
    }

    void setInData(NodeData nodeData, PortIndex const port) override
    {
        if (m_node) m_node->setInData(nodeData, port);
    }

    QWidget* embeddedWidget() override
    {
        return m_node ? m_node->embeddedWidget() : nullptr;
    }

    QJsonObject save() const override;

    void load(QJsonObject const& json) override;

public slots:

    void inputConnectionCreated(GtIntelliGraphNode::ConnectionId const& id) override
    {
        if (m_node) m_node->inputConnectionCreated(id);
    }

    void inputConnectionDeleted(GtIntelliGraphNode::ConnectionId const& id) override
    {
        if (m_node) m_node->inputConnectionDeleted(id);
    }

    void outputConnectionCreated(GtIntelliGraphNode::ConnectionId const& id) override
    {
        if (m_node) m_node->outputConnectionCreated(id);
    }

    void outputConnectionDeleted(GtIntelliGraphNode::ConnectionId const& id) override
    {
        if (m_node) m_node->outputConnectionDeleted(id);
    }

signals:

    void nodeInitialized();

private:

    QPointer<GtIntelliGraphNode> m_node;
};

#endif // GT_INTELLIGRAPHOBJECTMODEL_H
