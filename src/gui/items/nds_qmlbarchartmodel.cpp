#include <QQuickWidget>

#include "nds_qmlbarchartmodel.h"

NdsQmlBarChartModel::NdsQmlBarChartModel()
{
    m_qmlWid->setSource(QUrl("qrc:/qml/barchart.qml"));
    m_qmlWid->setResizeMode(QQuickWidget::SizeRootObjectToView);
}
