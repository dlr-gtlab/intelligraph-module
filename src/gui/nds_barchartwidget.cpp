#include <QPainter>

#include "qwt_plot_renderer.h"
#include "qwt_plot_multi_barchart.h"
#include "qwt_column_symbol.h"
#include "qwt_plot_layout.h"
#include "qwt_legend.h"
#include "qwt_scale_draw.h"
#include "qwt_text.h"
#include "qwt_math.h"
#include "qwt_scale_widget.h"
#include "qwt_text_label.h"
#include "qwt_plot_canvas.h"

#include "nds_barchartwidget.h"

NdsBarChartWidget::NdsBarChartWidget(QWidget* parent)
    : QwtPlot( parent )
{
    setAutoFillBackground( true );

//    setPalette( Qt::white );
//    canvas()->setPalette( QColor( "LemonChiffon" ) );

    setTitle( "Bar Chart" );

//    setAxisTitle( QwtAxis::YLeft, "Whatever" );
//    setAxisTitle( QwtAxis::XBottom, "Whatever" );

    m_barChartItem = new QwtPlotMultiBarChart( "Bar Chart " );
    m_barChartItem->setLayoutPolicy( QwtPlotMultiBarChart::AutoAdjustSamples );
    m_barChartItem->setSpacing( 20 );
    m_barChartItem->setMargin( 3 );

    m_barChartItem->attach( this );

    insertLegend(new QwtLegend(), QwtPlot::BottomLegend);

    populate();
    setOrientation( 0 );

    setMode(1);

    setFrameStyle(QFrame::NoFrame);
    setFrameShadow( QFrame::Plain );

    QwtPlotCanvas* can = qobject_cast<QwtPlotCanvas*>(canvas());

    if (can)
    {
        can->setFrameStyle(QFrame::NoFrame);
        can->setFrameShadow( QFrame::Plain );
    }

    setContentsMargins({10, 10, 10, 10});

    setAutoReplot( true );

    setAttribute(Qt::WA_TranslucentBackground);

}

void
NdsBarChartWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // smooth borders
    painter.setBrush(QBrush("#152231")); // visible color of background
    painter.setPen(Qt::transparent); // thin border color

    // Change border radius
    QRect rect = this->rect();
    rect.setWidth(rect.width()-1);
    rect.setHeight(rect.height()-1);
    painter.drawRoundedRect (rect, 15, 15);

    QwtPlot::paintEvent(event);
}

void
NdsBarChartWidget::populate()
{
    static const char* colors[] = { "#183380", "#2685bf", "#86cbe6" };

    const int numSamples = 5;
    const int numBars = sizeof( colors ) / sizeof( colors[0] );

    QList< QwtText > titles;
    for ( int i = 0; i < numBars; i++ )
    {
        QString title("Bar %1");
        titles += title.arg( i );
    }
    m_barChartItem->setBarTitles( titles );
    m_barChartItem->setLegendIconSize( QSize( 10, 5 ) );

    for ( int i = 0; i < numBars; i++ )
    {
        QwtColumnSymbol* symbol = new QwtColumnSymbol( QwtColumnSymbol::Box );
        symbol->setLineWidth( 1 );
        symbol->setFrameStyle( QwtColumnSymbol::NoFrame );
        symbol->setPalette( QPalette( colors[i] ) );

        m_barChartItem->setSymbol( i, symbol );
    }

    QVector< QVector< double > > series;
    for ( int i = 0; i < numSamples; i++ )
    {
        QVector< double > values;
        for ( int j = 0; j < numBars; j++ )
            values += ( 2 + qwtRand() % 8 );

        series += values;
    }

    m_barChartItem->setSamples( series );
}

void
NdsBarChartWidget::setMode( int mode )
{
    if ( mode == 0 )
    {
        m_barChartItem->setStyle( QwtPlotMultiBarChart::Grouped );
    }
    else
    {
        m_barChartItem->setStyle( QwtPlotMultiBarChart::Stacked );
    }
}

void
NdsBarChartWidget::setOrientation( int orientation )
{
    int axis1, axis2;

    if ( orientation == 0 )
    {
        axis1 = QwtAxis::XBottom;
        axis2 = QwtAxis::YLeft;

        m_barChartItem->setOrientation( Qt::Vertical );
    }
    else
    {
        axis1 = QwtAxis::YLeft;
        axis2 = QwtAxis::XBottom;

        m_barChartItem->setOrientation( Qt::Horizontal );
    }

    setAxisScale( axis1, 0, m_barChartItem->dataSize() - 1, 1.0 );
    setAxisAutoScale( axis2 );

    QwtScaleDraw* scaleDraw1 = axisScaleDraw( axis1 );
    scaleDraw1->enableComponent( QwtScaleDraw::Backbone, false );
    scaleDraw1->enableComponent( QwtScaleDraw::Ticks, false );

    QwtScaleDraw* scaleDraw2 = axisScaleDraw( axis2 );
    scaleDraw2->enableComponent( QwtScaleDraw::Backbone, true );
    scaleDraw2->enableComponent( QwtScaleDraw::Ticks, true );

    plotLayout()->setAlignCanvasToScale( axis1, true );
    plotLayout()->setAlignCanvasToScale( axis2, false );

    QFont font = axisWidget(QwtPlot::yLeft)->font();
    font.setPointSize(7);
    axisWidget(QwtPlot::yLeft)->setFont(font);

    font = axisWidget(QwtPlot::xBottom)->font();
    font.setPointSize(7);
    axisWidget(QwtPlot::xBottom)->setFont(font);

    font = titleLabel()->font();
    font.setPointSize(8);
    titleLabel()->setFont(font);

    font = legend()->font();
    font.setPointSize(7);
    legend()->setFont(font);

    plotLayout()->setCanvasMargin(5);
    updateCanvasMargins();

    replot();
}

void
NdsBarChartWidget::exportChart()
{
    QwtPlotRenderer renderer;
    renderer.exportTo( this, "barchart.pdf" );
}
