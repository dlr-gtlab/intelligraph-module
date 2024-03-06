/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 27.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#include "editabledoublelabel.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QDoubleValidator>
#include <QLabel>
#include <QLineEdit>

#include <gt_regexp.h>
#include <gt_logging.h>

namespace intelli
{
EditableDoubleLabel::EditableDoubleLabel(QString const& text,
                                         QWidget* parent) :
    QStackedWidget(parent),
    m_l(new QLabel(text)),
    m_e(new QLineEdit(text))
{
    addWidget(m_l);
    addWidget(m_e);

    m_l->installEventFilter(this);
    m_e->installEventFilter(this);
    m_e->setValidator(new QRegExpValidator(gt::re::forDoubles()));

    setMinimumWidth(30);

    connect(m_e, &QLineEdit::editingFinished,
            this, &EditableDoubleLabel::onTextChanged);
}

double
EditableDoubleLabel::value() const
{
    return m_l->text().toDouble();
}

void
EditableDoubleLabel::setValue(const double& value, bool emmit)
{
    if (m_l->text().toDouble() == value) return;


    m_l->setText(QString::number((value)));

    if (emmit) m_e->setText(QString::number((value)));
    else
    {
        /// text text of text edit without emit
        m_e->blockSignals(true);
        m_e->setText(QString::number((value)));
        m_e->blockSignals(false);
    }

}

bool
EditableDoubleLabel::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_e)
    {
        if(event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent *>(event);
            if(keyEvent->key() == Qt::Key_Return ||
                    keyEvent->key() == Qt::Key_Escape ||
                    keyEvent->key() == Qt::Key_Enter)
            {
                m_l->setText(m_e->text());
                this->setCurrentIndex(0);
            }
        }
        else if (event->type() == QEvent::FocusOut)
        {
            m_l->setText(m_e->text());
            this->setCurrentIndex(0);
        }
    }
    else if (watched == m_l)
    {
        if(event->type() == QEvent::MouseButtonDblClick){
            this->setCurrentIndex(1);
            m_e->setText(m_l->text());
            m_e->setFocus();
        }
    }
    return QWidget::eventFilter(watched, event);
}

QFont
EditableDoubleLabel::labelFont()
{
    if (m_l) return m_l->font();
    else return {};
}

void
EditableDoubleLabel::setLabelFont(const QFont& f)
{
    if (m_l) return m_l->setFont(f);
}

void
EditableDoubleLabel::setTextAlignment(Qt::Alignment textAlignment)
{
    if (m_l) m_l->setAlignment(textAlignment);
    if (m_e) m_e->setAlignment(textAlignment);
}

void
EditableDoubleLabel::onTextChanged()
{
    QString t = m_e->text();
    bool ok = true;
    double value = t.toDouble(&ok);

    if (!ok)
    {
        gtError() << tr("Problem to make %1 to a double value").arg(t);
        return;
    }

    setValue(value);

    emit valueChanged(value);
}
} // namespace intelli
