/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include "editableintegerlabel.h"

#include <gt_regexp.h>
#include <gt_logging.h>

#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QRegExpValidator>

using namespace intelli;

EditableBaseLabel::EditableBaseLabel(QString const& text,
                                     QWidget* parent) :
    QStackedWidget(parent),
    m_label(new QLabel(text)),
    m_edit(new QLineEdit(text))
{
    addWidget(m_label);
    addWidget(m_edit);

    m_label->installEventFilter(this);
    m_edit->installEventFilter(this);

    setMinimumWidth(30);

    connect(m_edit, &QLineEdit::editingFinished,
            this, &EditableBaseLabel::onTextEdited);
}

QLabel*
EditableBaseLabel::label() { return m_label; }
QLabel const*
EditableBaseLabel::label() const { return m_label; }

QLineEdit*
EditableBaseLabel::edit() { return m_edit; }
QLineEdit const*
EditableBaseLabel::edit() const{ return m_edit; }

QString
EditableBaseLabel::text() const
{
    return m_edit->text();
}

void
EditableBaseLabel::setText(QString const& text, bool emitSignal)
{
    m_edit->setText(text);
    m_label->setText(text + ' '); // padding

    if (emitSignal)
    {
        emit textChanged();
    }
}

void
EditableBaseLabel::setReadOnly(bool value)
{
    m_readOnly = true;
}

bool
EditableBaseLabel::readOnly() const
{
    return m_readOnly;
}

bool
EditableBaseLabel::eventFilter(QObject* watched, QEvent* event)
{
    if (m_readOnly)
    {
        this->setCurrentIndex(0);
        return QWidget::eventFilter(watched, event);
    }

    if (watched == m_edit)
    {
        if(event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent *>(event);
            if(keyEvent->key() == (Qt::Key_Return | Qt::Key_Escape | Qt::Key_Enter))
            {
                this->setCurrentIndex(0);
            }
        }
        else if (event->type() == QEvent::FocusOut)
        {
            this->setCurrentIndex(0);
        }
    }
    else if (watched == m_label)
    {
        if(event->type() == QEvent::MouseButtonDblClick)
        {
            this->setCurrentIndex(1);
            m_edit->setText(m_label->text().trimmed());
            m_edit->setFocus();
        }
    }
    return QWidget::eventFilter(watched, event);
}

void
EditableBaseLabel::setTextAlignment(Qt::Alignment textAlignment)
{
    edit()->setAlignment(textAlignment);
    label()->setAlignment(textAlignment);
}

void
EditableBaseLabel::onTextEdited()
{
    QString text = edit()->text();

    setText(std::move(text));
}

EditableIntegerLabel::EditableIntegerLabel(QString const& text, QWidget* parent) :
    EditableLabel<int>(text, parent)
{
    edit()->setValidator(new QRegExpValidator(QRegExp("-?[0-9]+")));
}

EditableDoubleLabel::EditableDoubleLabel(QString const& text, QWidget* parent) :
    EditableLabel<double>(text, parent)
{
    edit()->setValidator(new QRegExpValidator(gt::re::forDoubles()));
}
