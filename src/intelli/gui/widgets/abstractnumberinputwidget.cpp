/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include <intelli/gui/widgets/abstractnumberinputwidget.h>
#include <intelli/gui/widgets/editablelabel.h>

#include <gt_lineedit.h>
#include <gt_finally.h>
#include <gt_logging.h>

#include <QDial>
#include <QSlider>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>

using namespace intelli;

AbstractNumberInputWidget::AbstractNumberInputWidget(InputMode mode,
                                                     EditableLabel* low,
                                                     EditableLabel* high,
                                                     QWidget* parent) :
    QWidget(parent),
    m_mode(mode),
    m_low(low),
    m_high(high)
{
    assert(!layout());
    assert(low);
    assert(high);

    m_low->setParent(this);
    m_high->setParent(this);

    m_text = new GtLineEdit(this);
    m_text->installEventFilter(this);

    m_low->setMinimumWidth(40);
    m_high->setMinimumWidth(40);
    m_text->setMinimumWidth(75);

    m_low->setMaximumWidth(100);
    m_high->setMaximumWidth(100);

    m_low->setSizePolicy(m_low->sizePolicy().horizontalPolicy(), QSizePolicy::Fixed);
    m_high->setSizePolicy(m_high->sizePolicy().horizontalPolicy(), QSizePolicy::Fixed);
    m_text->setSizePolicy(m_text->sizePolicy().horizontalPolicy(), QSizePolicy::Fixed);

    m_low->setToolTip(tr("lower bound"));
    m_high->setToolTip(tr("upper bound"));

    m_dial = new QDial(this);
    m_dial->setTracking(true);
    m_dial->setContentsMargins(0, 0, 0, 0);
    m_dial->setNotchesVisible(true);
    m_dial->setSingleStep(1);

    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setTracking(true);
    m_slider->setContentsMargins(0, 0, 0, 0);
    m_slider->setSingleStep(1);

    auto setupEditableLabel = [this](EditableLabel* w){
        QFont f = w->label()->font();
        f.setItalic(true);
        w->label()->setFont(f);
        w->installEventFilter(this);
    };

    setupEditableLabel(m_low);
    setupEditableLabel(m_high);

    connect(m_low, &EditableLabel::textChanged,
            this, &AbstractNumberInputWidget::onMinEdited);
    connect(m_high, &EditableLabel::textChanged,
            this, &AbstractNumberInputWidget::onMaxEdited);
    connect(m_text, &GtLineEdit::editingFinished,
            this, &AbstractNumberInputWidget::onValueEdited);
    connect(m_dial, &QDial::valueChanged,
            this, &AbstractNumberInputWidget::commitSliderValueChange);
    connect(m_slider, &QSlider::valueChanged,
            this, &AbstractNumberInputWidget::commitSliderValueChange);
    connect(m_dial, &QDial::sliderReleased,
            this, &AbstractNumberInputWidget::valueComitted);

    applyInputMode(mode);
}

GtLineEdit*
AbstractNumberInputWidget::valueEdit() { return m_text; }
GtLineEdit const*
AbstractNumberInputWidget::valueEdit() const { return m_text; }

EditableLabel*
AbstractNumberInputWidget::low() { return m_low; }
EditableLabel const*
AbstractNumberInputWidget::low() const { return m_low; }

EditableLabel*
AbstractNumberInputWidget::high() { return m_high; }
EditableLabel const*
AbstractNumberInputWidget::high() const { return m_high; }

QDial*
AbstractNumberInputWidget::dial() { return m_dial; }
QDial const*
AbstractNumberInputWidget::dial() const { return m_dial; }

QSlider*
AbstractNumberInputWidget::slider() { return m_slider; }
QSlider const*
AbstractNumberInputWidget::slider() const { return m_slider; }

void
AbstractNumberInputWidget::applyInputMode(InputMode mode)
{
    auto setupVLayout = [this](){
        delete layout();
        auto* lay = new QVBoxLayout(this);

        lay->addWidget(m_dial);
        lay->addWidget(m_slider);
        lay->setContentsMargins(0, 0, 0, 0);
        auto* innerlay = new QHBoxLayout();
        innerlay->addWidget(m_low);
        innerlay->addWidget(m_text);
        innerlay->addWidget(m_high);
        innerlay->setContentsMargins(0, 0, 0, 0);
        lay->addLayout(innerlay);
        setLayout(lay);
    };

    auto setupHLayout = [this](){
        delete layout();
        auto* lay = new QHBoxLayout(this);

        lay->addWidget(m_dial);
        lay->addWidget(m_slider);
        lay->setContentsMargins(0, 0, 0, 0);
        auto* innerlay = new QVBoxLayout();
        innerlay->addWidget(m_high);
        innerlay->addStretch();
        innerlay->addWidget(m_text);
        innerlay->addStretch();
        innerlay->addWidget(m_low);
        innerlay->setContentsMargins(0, 0, 0, 0);
        lay->addLayout(innerlay);
        setLayout(lay);
    };

    constexpr int padding = 10;

    m_useBounds = true;
    m_mode = mode;

    switch (mode)
    {
    default:
    case LineEditUnbound:
        m_useBounds = false;

        setupVLayout();
        m_dial->setHidden(true);
        m_slider->setHidden(true);
        m_low->setHidden(true);
        m_high->setHidden(true);

        setMinimumHeight(m_text->minimumSizeHint().height());
        break;
    case LineEditBound:
        m_useBounds = true;

        setupVLayout();
        m_dial->setHidden(true);
        m_slider->setHidden(true);
        m_low->setHidden(false);
        m_high->setHidden(false);

        m_low->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_high->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        setMinimumHeight(m_text->minimumSizeHint().height());
        break;
    case Dial:
        setupVLayout();
        m_dial->setHidden(false);
        m_slider->setHidden(true);
        m_low->setHidden(false);
        m_high->setHidden(false);

        m_low->setTextAlignment(Qt::AlignCenter);
        m_high->setTextAlignment(Qt::AlignCenter);

        setMinimumHeight(m_text->minimumSizeHint().height() +
                         m_dial->minimumSizeHint().height() +
                         2 * padding);
        break;
    case SliderH:
        setupVLayout();
        m_dial->setHidden(true);
        m_slider->setHidden(false);
        m_slider->setOrientation(Qt::Horizontal);
        m_low->setHidden(false);
        m_high->setHidden(false);

        m_low->setTextAlignment(Qt::AlignLeft   | Qt::AlignVCenter);
        m_high->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        setMinimumHeight(m_text->minimumSizeHint().height() +
                         m_slider->minimumSizeHint().height() +
                         padding);
        break;
    case SliderV:
        setupHLayout();
        m_dial->setHidden(true);
        m_slider->setHidden(false);
        m_slider->setOrientation(Qt::Vertical);
        m_low->setHidden(false);
        m_high->setHidden(false);

        m_low->setTextAlignment(Qt::AlignLeft  | Qt::AlignVCenter);
        m_high->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        setMinimumHeight(m_slider->sizeHint().height());
        break;
    }

    resize(minimumSizeHint());
}

void
AbstractNumberInputWidget::setInputMode(InputMode mode)
{
    if (m_mode == mode) return;

    applyInputMode(mode);

    commitValueChange(); // bounds may have changed
    emit valueChanged();
    emit valueComitted();
}

AbstractNumberInputWidget::InputMode
AbstractNumberInputWidget::inputMode() const
{
    return m_mode;
}

void
AbstractNumberInputWidget::setRange(QVariant const& valueV,
                                    QVariant const& minV,
                                    QVariant const& maxV)
{
    {
        QSignalBlocker blocker(*this);

        applyRange(valueV, minV, maxV);
    }

    if (inputMode() != LineEditUnbound)
    {
        m_low->setVisible(this->useBounds());
        m_high->setVisible(this->useBounds());
    }

    emit minChanged();
    emit maxChanged();
    emit valueChanged();
    emit valueComitted();
}

QString
AbstractNumberInputWidget::value() const
{
    return m_text->text();
}

bool
AbstractNumberInputWidget::useBounds() const
{
    return m_useBounds;
}

void
AbstractNumberInputWidget::onValueEdited()
{
    commitValueChange();
    emit valueChanged();
    emit valueComitted();
}

void
AbstractNumberInputWidget::onMinEdited()
{
    if (useBounds())
    {
        commitMinValueChange();
        emit minChanged();
    }
}

void
AbstractNumberInputWidget::onMaxEdited()
{
    if (useBounds())
    {
        commitMaxValueChange();
        emit maxChanged();
    }
}

bool
AbstractNumberInputWidget::eventFilter(QObject* obj, QEvent* e)
{
    if (qobject_cast<EditableLabel*>(obj))
    {
        switch (e->type())
        {
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
            return true;
        default:
            return false;
        }
    }
    // pass the event on to the parent class
    return QWidget::eventFilter(obj, e);
}

