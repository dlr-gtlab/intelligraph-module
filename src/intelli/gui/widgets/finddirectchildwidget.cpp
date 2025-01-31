/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include <intelli/gui/widgets/finddirectchildwidget.h>

#include <QVBoxLayout>
#include <QCompleter>

#include <gt_objectfactory.h>
#include <gt_lineedit.h>
#include <gt_stringproperty.h>
#include <gt_application.h>

using namespace intelli;

FindDirectChildWidget::FindDirectChildWidget(QWidget* parent) :
    QWidget(parent)
{
    auto* lay = new QVBoxLayout;
    setLayout(lay);

    auto* classNameCompleter =
        new QCompleter(gtObjectFactory->knownClasses());
    classNameCompleter->setCompletionMode(QCompleter::InlineCompletion);

    m_classNameEdit = new GtLineEdit;
    m_classNameEdit->setPlaceholderText(QStringLiteral("class name"));
    m_classNameEdit->setCompleter(classNameCompleter);

    m_objectNameEdit = new GtLineEdit;
    m_objectNameEdit->setPlaceholderText(QStringLiteral("object name"));

    lay->addWidget(m_objectNameEdit);
    lay->addWidget(m_classNameEdit);
    lay->setContentsMargins(0, 0, 0, 0);

    m_objectNameEdit->setFixedHeight(16);
    m_classNameEdit->setFixedHeight(16);

    setMinimumWidth(120);
    if (!gtApp->devMode())
    {
        m_classNameEdit->hide();
    }

    connect(m_classNameEdit, SIGNAL(focusOut()),
            this, SLOT(reactOnClassNameWidgetChange()));
    connect(m_classNameEdit, SIGNAL(clearFocusOut()),
            this, SLOT(reactOnClassNameWidgetChange()));

    connect(m_objectNameEdit, SIGNAL(focusOut()),
            this, SLOT(reactOnObjectNameWidgetChange()));
    connect(m_objectNameEdit, SIGNAL(clearFocusOut()),
            this, SLOT(reactOnObjectNameWidgetChange()));

    resize(minimumSizeHint());
}

void
FindDirectChildWidget::setClassNameWidget(QString const& className)
{
    if (m_classNameEdit) m_classNameEdit->setText(className);
}

void
FindDirectChildWidget::setObjectNameWidget(QString const& objectName)
{
    if (m_objectNameEdit) m_objectNameEdit->setText(objectName);
}

void
FindDirectChildWidget::updateNameCompleter(ObjectData const* data)
{
    QStringList allChildrenNames;

    if (data)
    {
        if (auto* obj = data->object())
        {
            for (auto const c : obj->findDirectChildren())
            {
                allChildrenNames.append(c->objectName());
            }
        }
    }

    if (!allChildrenNames.isEmpty())
    {
        auto* objectNameCompleter = new QCompleter(allChildrenNames);
        objectNameCompleter->setCompletionMode(QCompleter::InlineCompletion);
        m_objectNameEdit->setCompleter(objectNameCompleter);
    }
}

void
FindDirectChildWidget::reactOnClassNameWidgetChange()
{
    if (!m_classNameEdit) return;
    emit updateClass(m_classNameEdit->text());
}

void
FindDirectChildWidget::reactOnObjectNameWidgetChange()
{
    if (!m_objectNameEdit) return;
    emit updateObjectName(m_objectNameEdit->text());
}


void
FindDirectChildWidget::updateClassText()
{
    if (!m_classNameEdit) return;

    auto* prop = qobject_cast<GtStringProperty*>(sender());

    if (!prop) return;

    m_classNameEdit->setText(prop->getVal());
}

void
FindDirectChildWidget::updateNameText()
{
    if (!m_objectNameEdit) return;

    auto* prop = qobject_cast<GtStringProperty*>(sender());

    if (!prop) return;

    m_objectNameEdit->setText(prop->getVal());
}
