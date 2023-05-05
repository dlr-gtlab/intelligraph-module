#ifndef GT_IGABSTRACTQWTNODE_H
#define GT_IGABSTRACTQWTNODE_H

#include "gt_intelligraphnode.h"
#include "gt_igvolatileptr.h"

#include "qwt_plot.h"

class GtIgAbstractQwtNode : public GtIntelliGraphNode
{
    Q_OBJECT

public:

    GtIgAbstractQwtNode(QString const& caption, GtObject* parent = nullptr);

    unsigned int nPorts(PortType const portType) const override;

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override;

    NodeData outData(PortIndex const port) override;

    void setInData(NodeData nodeData, PortIndex const port) override;

    QWidget* embeddedWidget() override
    {
        if (!m_plot) initWidget();
        return m_plot;
    }

protected:

    gt::ig::volatile_ptr<QwtPlot> m_plot;

    virtual void initWidget() = 0;

private:

    NodeData _nodeData;
};

#endif // GT_IGABSTRACTQWTNODE_H
