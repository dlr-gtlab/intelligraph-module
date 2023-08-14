/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_OBJECTMODEL_H
#define GT_INTELLI_OBJECTMODEL_H

#include "intelli/node.h"
#include "intelli/nodedata.h"

#include <QtNodes/NodeDelegateModel>

#include <QPointer>

namespace intelli
{

class ObjectModel final : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:

    using QtNodeEvalState    = QtNodes::NodeEvalState;
    using QtNodeFlag         = QtNodes::NodeFlag;
    using QtNodeFlags        = QtNodes::NodeFlags;
    using QtPortType         = QtNodes::PortType;
    using QtPortIndex        = QtNodes::PortIndex;
    using QtNodeDataType     = QtNodes::NodeDataType;
    using QtNodeData         = std::shared_ptr<QtNodes::NodeData>;
    using QtConnectionPolicy = QtNodes::ConnectionPolicy;
    using QtConnectionId     = QtNodes::ConnectionId;

    explicit ObjectModel(QString const& className);
    explicit ObjectModel(Node& node);

    static PortType cast_port_type(QtPortType type) { return static_cast<PortType>(type); }
    static QtPortType cast_port_type(PortType type) { return static_cast<QtPortType>(type); }

    /// initializes the model with a new node object
    void init(Node& node);
    
    Node* node() { return m_node; }
    Node const* node() const { return m_node; }

    QtNodeFlags flags() const override;

    QtNodeEvalState evalState() const override;

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
    
    QPointer<Node> m_node;

    bool m_evaluating = false;
};

class ObjectModelData : public QtNodes::NodeData
{
public:
    
    explicit ObjectModelData(Node::NodeDataPtr data = nullptr)
        : m_data(std::move(data))
    { }

    QtNodes::NodeDataType type() const override
    {
        if (!m_data) return {};

        return {m_data->typeId(), m_data->typeId()};
    }
    
    Node::NodeDataPtr const& data() const { return m_data; }

private:
    
    Node::NodeDataPtr m_data;
};

} // namespace intelli

using GtIntelliGraphObjectModel [[deprecated]] = intelli::ObjectModel;

#endif // GT_INTELLI_OBJECTMODEL_H
