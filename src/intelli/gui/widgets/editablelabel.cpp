/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include <intelli/gui/widgets/editablelabel.h>

#include <gt_regexp.h>
#include <gt_logging.h>

#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QRegExpValidator>

using namespace intelli;

EditableLabel::EditableLabel(QString const& text,
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
            this, &EditableLabel::onTextEdited);
}

QLabel*
EditableLabel::label() { return m_label; }
QLabel const*
EditableLabel::label() const { return m_label; }

QLineEdit*
EditableLabel::edit() { return m_edit; }
QLineEdit const*
EditableLabel::edit() const{ return m_edit; }

QString
EditableLabel::text() const
{
    return m_edit->text();
}

void
EditableLabel::setText(QString const& text, bool emitSignal)
{
    m_edit->setText(text);
    m_label->setText(text + ' '); // padding

    if (emitSignal)
    {
        emit textChanged();
    }
}

void
EditableLabel::setReadOnly(bool value)
{
    m_readOnly = true;
}

bool
EditableLabel::readOnly() const
{
    return m_readOnly;
}

bool
EditableLabel::eventFilter(QObject* watched, QEvent* event)
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
EditableLabel::setTextAlignment(Qt::Alignment textAlignment)
{
    edit()->setAlignment(textAlignment);
    label()->setAlignment(textAlignment);
}

void
EditableLabel::onTextEdited()
{
    QString text = edit()->text();

    setText(std::move(text));
}

EditableIntegerLabel::EditableIntegerLabel(QString const& text, QWidget* parent) :
    EditableNumberLabel<int>(text, parent)
{
    edit()->setValidator(new QRegExpValidator(QRegExp("-?[0-9]+")));
}

EditableDoubleLabel::EditableDoubleLabel(QString const& text, QWidget* parent) :
    EditableNumberLabel<double>(text, parent)
{
    edit()->setValidator(new QRegExpValidator(gt::re::forDoubles()));
}
