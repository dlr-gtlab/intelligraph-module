#ifndef GT_IGOBJECTMEMENTONODE_H
#define GT_IGOBJECTMEMENTONODE_H

#include "gt_intelligraphnode.h"
#include "gt_igvolatileptr.h"

#include "gt_codeeditor.h"

class GtIgObjectData;
class GT_IG_EXPORT GtIgObjectMementoNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgObjectMementoNode();

    unsigned int nPorts(PortType const type) const override;

    NodeDataType dataType(PortType const type, PortIndex const idx) const override;

    NodeData outData(PortIndex const port) override;

    void setInData(NodeData data, PortIndex const port) override;

    QWidget* embeddedWidget() override;

private:
    std::shared_ptr<GtIgObjectData> m_data;

    gt::ig::volatile_ptr<GtCodeEditor> m_editor;

    void initWidget();
};

#endif // GT_IGOBJECTMEMENTONODE_H
