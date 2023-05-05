#ifndef NDSBARCHARTWIDGET_H
#define NDSBARCHARTWIDGET_H

#include "qwt_plot.h"

class QwtPlotMultiBarChart;

class NdsBarChartWidget : public QwtPlot
{
    Q_OBJECT

public:

    NdsBarChartWidget(QWidget* = nullptr);

    ~NdsBarChartWidget() = default;

    void paintEvent(QPaintEvent *event) override;

public Q_SLOTS:

    void setMode( int );
    void setOrientation( int );
    void exportChart();

private:

    void populate();

    QwtPlotMultiBarChart* m_barChartItem;

};

#endif // NDSBARCHARTWIDGET_H
