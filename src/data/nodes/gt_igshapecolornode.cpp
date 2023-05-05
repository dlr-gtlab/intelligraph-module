#include <QFormLayout>
#include <QEvent>
#include <QColorDialog>
#include <QUuid>

#include "gt_igshapecolornode.h"

GTIG_REGISTER_NODE(GtIgShapeColorNode)

GtIgShapeColorNode::GtIgShapeColorNode() :
    GtIgAbstractShapeNode(tr("Shape Color")),
    m_color("color", tr("Color"), tr("Color"), Qt::lightGray)
{
    registerProperty(m_color);
}

void
GtIgShapeColorNode::compute(QList<ShapePtr>const& shapesIn,
                            QList<ShapePtr>& shapesOut)
{
    for (const ShapePtr& shape : shapesIn)
    {
        ShapePtr shapeCpy = shape->getCopy();
        shapeCpy->setColor(m_color);
        shapeCpy->setUuid(QUuid::createUuid().toString());
        shapesOut << std::move(shapeCpy);
    }
}

bool
GtIgShapeColorNode::eventFilter(QObject* object, QEvent* event)
{
    if (object == m_editor && event->type() == QEvent::MouseButtonPress)
    {
        QColor color = QColorDialog::getColor(m_color);

        if (color.isValid())
        {
            m_color = color;
            setWidgetColor();

            emit dataUpdated(0);
        }
    }

    return false;
}

void
GtIgShapeColorNode::setWidgetColor()
{
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, m_color);

    m_editor->setAutoFillBackground(true);
    m_editor->setPalette(pal);
}

void
GtIgShapeColorNode::initWidget()
{
    m_editor = gt::ig::make_volatile<QWidget>();

    auto* lay = new QFormLayout;

    m_editor->setLayout(lay);
    m_editor->setMinimumWidth(70);

    m_editor->installEventFilter(this);

    setWidgetColor();
}
