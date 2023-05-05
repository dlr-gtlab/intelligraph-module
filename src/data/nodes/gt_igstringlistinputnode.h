/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_IGSTRINGLISTINPUTNODE_H
#define GT_IGSTRINGLISTINPUTNODE_H

#include "gt_intelligraphnode.h"
#include "gt_igvolatileptr.h"

#include "gt_propertystructcontainer.h"

#include <QTextEdit>

class GtIgStringListData;
class GtIgStringListInputNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgStringListInputNode();

    unsigned int nPorts(PortType const type) const override;

    NodeDataType dataType(PortType const type, PortIndex const idx) const override;

    NodeData outData(PortIndex const port) override;

    QWidget* embeddedWidget() override;

public slots:

    void updateNode() override;

private:

    GtPropertyStructContainer m_values;

    gt::ig::volatile_ptr<QTextEdit> m_editor;

    QStringList values() const;

    void initWidget();
};

#endif // GT_IGSTRINGLISTINPUTNODE_H
