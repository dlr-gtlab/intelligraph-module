#include <QQuickWidget>

#include "ndsqmllinechartmodel.h"

NdsQmlLineChartModel::NdsQmlLineChartModel()
{
    m_qmlWid->setSource(QUrl("qrc:/qml/linechart.qml"));
    m_qmlWid->setResizeMode(QQuickWidget::SizeRootObjectToView);
}
