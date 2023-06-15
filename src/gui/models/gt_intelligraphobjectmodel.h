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


class GtIntelliGraphObjectModel final : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:

    using NodeFlag         = gt::ig::NodeFlag;
    using PortType         = QtNodes::PortType;
    using PortIndex        = QtNodes::PortIndex;
    using NodeDataType     = QtNodes::NodeDataType;
    using NodeData         = std::shared_ptr<QtNodes::NodeData>;
    using ConnectionPolicy = QtNodes::ConnectionPolicy;
    using ConnectionId     = QtNodes::ConnectionId;

    explicit GtIntelliGraphObjectModel(QString const& className);

    static gt::ig::PortType cast_port_type(PortType type) { return static_cast<gt::ig::PortType>(type); }

    /// initializes the model with a new node object
    void init(GtIntelliGraphNode& node);

    GtIntelliGraphNode& node() { assert(m_node); return *m_node; }
    GtIntelliGraphNode const& node() const { assert(m_node); return *m_node; }

    bool resizable() const override;

    bool captionVisible() const override;

    QString caption() const override;

    QString name() const override;

    unsigned nPorts(PortType const type) const override;

    NodeDataType dataType(PortType const type, PortIndex const idx) const override;

    bool portCaptionVisible(PortType type, PortIndex idx) const override;

    QString portCaption(PortType type, PortIndex idx) const override;

    NodeData outData(PortIndex const port) override;

    void setInData(NodeData nodeData, PortIndex const port) override;

    QWidget* embeddedWidget() override;

    QJsonObject save() const override;

    void load(QJsonObject const& json) override;

public slots:

    void outputConnectionCreated(ConnectionId const &) override;

    void outputConnectionDeleted(ConnectionId const &) override;

signals:

    void nodeInitialized();

private:

    QPointer<GtIntelliGraphNode> m_node;
};

class GtIgObjectModelData : public QtNodes::NodeData
{
public:

    explicit GtIgObjectModelData(GtIntelliGraphNode::NodeData data = nullptr)
        : m_data(std::move(data))
    { }

    QtNodes::NodeDataType type() const override
    {
        if (!m_data) return {};

        return {m_data->typeId(), m_data->typeId()};
    }

    GtIntelliGraphNode::NodeData const& data() const { return m_data; }

private:

    GtIntelliGraphNode::NodeData m_data;
};
#endif // GT_INTELLIGRAPHOBJECTMODEL_H
