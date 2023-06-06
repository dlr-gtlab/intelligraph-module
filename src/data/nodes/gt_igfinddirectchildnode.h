/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 5.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_IGFINDDIRECTCHILDNODE_H
#define GT_IGFINDDIRECTCHILDNODE_H

#include "gt_intelligraphnode.h"
#include "gt_igvolatileptr.h"

#include "gt_stringproperty.h"
#include "gt_lineedit.h"

class GtIgObjectData;
class GtIgFindDirectChildNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgFindDirectChildNode();

    unsigned int nPorts(PortType const type) const override;

    NodeDataType dataType(PortType const type, PortIndex const idx) const override;

    NodeData outData(PortIndex const port) override;

    void setInData(NodeData data, PortIndex const port) override;

    QWidget* embeddedWidget() override;

public slots:

    void updateNode() override;

private:
    std::shared_ptr<GtIgObjectData> m_parent;

    gt::ig::volatile_ptr<GtLineEdit> m_editor;

    GtStringProperty m_childClassName;

    void initWidget();
};

#endif // GT_IGFINDDIRECTCHILDNODE_H
