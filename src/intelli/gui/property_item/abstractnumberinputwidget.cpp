/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 05.03.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#include "abstractnumberinputwidget.h"

#include <QMouseEvent>
#include "editabledoublelabel.h"
#include "editableintegerlabel.h"

namespace intelli
{

AbstractNumberInputWidget::AbstractNumberInputWidget(QWidget* parent) :
    QWidget(parent)
{

}

AbstractNumberInputWidget::InputType AbstractNumberInputWidget::typeFromString(
        QString const& typeString)
{
    AbstractNumberInputWidget::InputType t = AbstractNumberInputWidget::Dial;
    if (typeString == "dial")
    {
        t = AbstractNumberInputWidget::Dial;
    }
    else if (typeString == "sliderH")
    {
        t = AbstractNumberInputWidget::SliderH;
    }
    else if (typeString == "sliderV")
    {
        t = AbstractNumberInputWidget::SliderV;
    }
    else if (typeString == "Text")
    {
        t = AbstractNumberInputWidget::LineEdit;
    }
    return t;
}

QLayout*
AbstractNumberInputWidget::newDialLayout(QWidget* slider, QWidget* minText,
                                         QWidget* valueText, QWidget* maxText)
{
    if (!slider || !minText || !valueText || !maxText) return nullptr;

    auto* layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(slider);

    auto* textLay = new QHBoxLayout;
    textLay->addWidget(minText, Qt::AlignHCenter);
    textLay->addWidget(valueText, Qt::AlignHCenter);
    textLay->addWidget(maxText, Qt::AlignHCenter);

    minText->setHidden(false);
    maxText->setHidden(false);

    layout->addLayout(textLay);

    return layout;
}

QLayout*
AbstractNumberInputWidget::newSliderHLayout(QWidget* slider, QWidget* minText,
                                            QWidget* valueText,
                                            QWidget* maxText)
{
    if (!slider || !minText || !valueText || !maxText) return nullptr;

    minText->setHidden(false);
    maxText->setHidden(false);

    auto* layout = new QVBoxLayout;
    layout->addWidget(slider);

    auto* textLayout = new QHBoxLayout;
    textLayout->addWidget(minText, Qt::AlignHCenter);
    textLayout->addWidget(valueText, Qt::AlignHCenter);
    textLayout->addWidget(maxText, Qt::AlignHCenter);

    layout->addLayout(textLayout);

    return layout;
}

QLayout*
AbstractNumberInputWidget::newSliderVLayout(QWidget* slider,
                                            QWidget* minText,
                                            QWidget* valueText,
                                            QWidget* maxText)
{
    if (!slider || !minText || !valueText || !maxText) return nullptr;

    minText->setHidden(false);
    maxText->setHidden(false);

    auto* layout = new QHBoxLayout;
    layout->addWidget(slider);

    auto* textLayout = new QVBoxLayout;
    textLayout->addWidget(minText, Qt::AlignLeft);
    textLayout->addWidget(valueText, Qt::AlignLeft);
    textLayout->addWidget(maxText, Qt::AlignLeft);

    layout->addLayout(textLayout);

    return layout;
}

void
AbstractNumberInputWidget::resizeEvent(QResizeEvent *event)
{
    emit sizeChanged();

    QWidget::resizeEvent(event);
}

bool
AbstractNumberInputWidget::eventFilter(QObject* obj, QEvent* e)
{
    if (qobject_cast<intelli::EditableDoubleLabel*>(obj) ||
            qobject_cast<intelli::EditableIntegerLabel*>(obj))
    {
        if (e->type() == QEvent::MouseMove ||
                e->type() == QEvent::MouseButtonPress ||
                e->type() == QEvent::MouseButtonRelease)
        {
            return true;
        }
        else return false;
    }
    else
    {
        // pass the event on to the parent class
        return QWidget::eventFilter(obj, e);
    }
}

}
