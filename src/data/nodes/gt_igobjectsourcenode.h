#ifndef GT_IGOBJECTSOURCENODE_H
#define GT_IGOBJECTSOURCENODE_H

#include "gt_intelligraphnode.h"

#include "gt_igvolatileptr.h"
#include "gt_igobjectlinkproperty.h"

#include "gt_propertyobjectlinkeditor.h"

class GtIgObjectData;
class GtIgStringListData;
class GT_IG_EXPORT GtIgObjectSourceNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgObjectSourceNode();

    unsigned int nPorts(PortType const type) const override;

    NodeDataType dataType(PortType const type, PortIndex const idx) const override;

    NodeData outData(PortIndex const port) override;

    void setInData(NodeData data, PortIndex const port) override;

    QWidget* embeddedWidget() override;

public slots:

    void updateNode() override;

private:

    GtIgObjectLinkProperty m_object;

    /// member to keep track of last object pointer. Used to disconnect if
    /// object changes
    QPointer<GtObject> m_lastObject;

    gt::ig::volatile_ptr<GtPropertyObjectLinkEditor> m_editor;

    void initWidget();
};

#endif // GT_IGOBJECTSOURCENODE_H
