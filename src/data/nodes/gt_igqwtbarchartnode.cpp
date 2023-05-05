#include "nds_barchartwidget.h"

#include "gt_igqwtbarchartnode.h"

GTIG_REGISTER_NODE(GtIgQwtBarChartNode);

GtIgQwtBarChartNode::GtIgQwtBarChartNode() :
    GtIgAbstractQwtNode(tr("Bar Chart"))
{ }

void
GtIgQwtBarChartNode::initWidget()
{
    m_plot = gt::ig::make_volatile<NdsBarChartWidget>();
    m_plot->setMinimumSize({400, 250});
}
