#include "nds_barchartwidget.h"

#include "nds_qwtbarchartmodel.h"

NdsQwtBarChartModel::NdsQwtBarChartModel()
{
    m_plot = new NdsBarChartWidget();
    m_plot->setMinimumSize({400, 250});
}
