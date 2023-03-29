#include <QFormLayout>
#include <QEvent>
#include <QColorDialog>

#include "ndsshapecolormodel.h"

NdsShapeColorModel::NdsShapeColorModel() :
    m_mainWid(new QWidget())
{
    m_color = Qt::green;

    auto* lay = new QFormLayout;

    m_mainWid->setLayout(lay);
    m_mainWid->setMinimumWidth(70);

    m_mainWid->installEventFilter(this);

    setWidgetColor();
}

QWidget*
NdsShapeColorModel::embeddedWidget()
{
    return m_mainWid;
}

void
NdsShapeColorModel::compute(const QList<ShapePtr> &shapesIn,
                                 QList<ShapePtr> &shapesOut)
{
    foreach (const ShapePtr shape, shapesIn)
    {
        ShapePtr shapeCpy = shape->getCopy();
        shapeCpy->setColor(m_color);
        shapeCpy->setUuid(QUuid::createUuid().toString());
        shapesOut << shapeCpy;
    }
}

bool
NdsShapeColorModel::eventFilter(QObject* object, QEvent* event)
{
    if (object == m_mainWid)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QColor color = QColorDialog::getColor(m_color);

            if (color.isValid())
            {
                m_color = color;
                setWidgetColor();

                Q_EMIT dataUpdated(0);
            }
        }
    }

    return false;
}

void
NdsShapeColorModel::setWidgetColor()
{
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, m_color);

    m_mainWid->setAutoFillBackground(true);
    m_mainWid->setPalette(pal);

}
