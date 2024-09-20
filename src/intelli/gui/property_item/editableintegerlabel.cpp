/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include "editableintegerlabel.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QRegExpValidator>
#include <QLabel>
#include <QLineEdit>

#include <gt_regexp.h>
#include <gt_logging.h>

namespace intelli
{

EditableIntegerLabel::EditableIntegerLabel(QString const& text,
                                           QWidget* parent) :
    QStackedWidget(parent),
    m_l(new QLabel(text)),
    m_e(new QLineEdit(text))
{
    addWidget(m_l);
    addWidget(m_e);

    m_l->installEventFilter(this);
    m_e->installEventFilter(this);
    m_e->setValidator(new QRegExpValidator(QRegExp("-?[0-9]+")));

    setMinimumWidth(30);

    connect(m_e, &QLineEdit::editingFinished,
            this, &EditableIntegerLabel::onTextChanged);
}

int
EditableIntegerLabel::value() const
{
    return m_l->text().toInt();
}

void
EditableIntegerLabel::setValue(const int& value, bool emmit)
{
    if (m_l->text().toInt() == value) return;


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
EditableIntegerLabel::eventFilter(QObject* watched, QEvent* event)
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
EditableIntegerLabel::labelFont()
{
    if (m_l) return m_l->font();
    else return {};
}

void
EditableIntegerLabel::setLabelFont(const QFont& f)
{
    if (m_l) return m_l->setFont(f);
}

void
EditableIntegerLabel::setTextAlignment(Qt::Alignment textAlignment)
{
    if (m_l) m_l->setAlignment(textAlignment);
    if (m_e) m_e->setAlignment(textAlignment);
}

void
EditableIntegerLabel::onTextChanged()
{
    QString t = m_e->text();
    bool ok = true;
    int value = t.toInt(&ok);

    if (!ok)
    {
        gtError() << tr("Problem to make %1 to a double value").arg(t);
        return;
    }

    setValue(value);

    emit valueChanged(value);
}
} // namespace intelli
