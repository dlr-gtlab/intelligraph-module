#include <QQuickWidget>

#include "nds_qmlpiechartmodel.h"

NdsQmlPieChartModel::NdsQmlPieChartModel()
{
    m_qmlWid->setSource(QUrl("qrc:/qml/piechart.qml"));
    m_qmlWid->setResizeMode(QQuickWidget::SizeRootObjectToView);
}
