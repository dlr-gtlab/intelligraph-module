#ifndef GT_IGOBJECTSOURCENODE_H
#define GT_IGOBJECTSOURCENODE_H

#include "gt_intelligraphnode.h"

#include "gt_igvolatileptr.h"
#include "gt_igobjectlinkproperty.h"

#include "gt_propertyobjectlinkeditor.h"

class GtIgObjectData2;
class GtIgStringListData2;
class GtIgObjectSourceNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgObjectSourceNode();

protected:

    NodeData eval(PortId outId) override;

private:

    /// selected object
    GtIgObjectLinkProperty m_object;

    /// member to keep track of last object pointer. Used to disconnect if
    /// object changes
    QPointer<GtObject> m_lastObject;

    PortId m_inPort, m_outPort;
};

#endif // GT_IGOBJECTSOURCENODE_H
