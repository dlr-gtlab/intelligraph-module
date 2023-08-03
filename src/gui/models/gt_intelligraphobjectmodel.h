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
#include "gt_ignodedata.h"

#include <QtNodes/NodeDelegateModel>


class GtIntelliGraphObjectModel final : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:

    using NodeFlag           = gt::ig::NodeFlag;
    using QtNodeFlag         = QtNodes::NodeFlag;
    using QtNodeFlags        = QtNodes::NodeFlags;
    using QtPortType         = QtNodes::PortType;
    using QtPortIndex        = QtNodes::PortIndex;
    using QtNodeDataType     = QtNodes::NodeDataType;
    using QtNodeData         = std::shared_ptr<QtNodes::NodeData>;
    using QtConnectionPolicy = QtNodes::ConnectionPolicy;
    using QtConnectionId     = QtNodes::ConnectionId;

    explicit GtIntelliGraphObjectModel(QString const& className);
    explicit GtIntelliGraphObjectModel(GtIntelliGraphNode& node);

    static gt::ig::PortType cast_port_type(QtPortType type) { return static_cast<gt::ig::PortType>(type); }
    static QtPortType cast_port_type(gt::ig::PortType type) { return static_cast<QtPortType>(type); }

    /// initializes the model with a new node object
    void init(GtIntelliGraphNode& node);

    GtIntelliGraphNode* node() { return m_node; }
    GtIntelliGraphNode const* node() const { return m_node; }

    QtNodeFlags flags() const override;

    bool captionVisible() const override;

    QString caption() const override;

    QString name() const override;

    unsigned nPorts(QtPortType const type) const override;

    QtNodeDataType dataType(QtPortType const type, QtPortIndex const idx) const override;

    bool portCaptionVisible(QtPortType type, QtPortIndex idx) const override;

    QString portCaption(QtPortType type, QtPortIndex idx) const override;

    QtNodeData outData(QtPortIndex const port) override;

    void setInData(QtNodeData nodeData, QtPortIndex const port) override;

    QWidget* embeddedWidget() override;

    QJsonObject save() const override;

    void load(QJsonObject const& json) override;

public slots:

    void outputConnectionCreated(QtConnectionId const &) override;

    void outputConnectionDeleted(QtConnectionId const &) override;

signals:

    void nodeInitialized();

private:

    QPointer<GtIntelliGraphNode> m_node;

    bool m_evaluating = false;
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
