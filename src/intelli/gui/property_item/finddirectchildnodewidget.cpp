/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 07.06.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#include "finddirectchildnodewidget.h"

#include <QVBoxLayout>
#include <QCompleter>

#include <gt_objectfactory.h>
#include <gt_lineedit.h>
#include <gt_stringproperty.h>
#include <gt_application.h>

namespace intelli
{
FindDirectChildNodeWidget::FindDirectChildNodeWidget(QWidget* parent) :
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


    setMinimumWidth(120);
    if (!gtApp->devMode())
    {
        m_classNameEdit->hide();
        setFixedHeight(40);
    }
    else
    {
        setFixedHeight(65);
    }

    connect(m_classNameEdit, SIGNAL(focusOut()),
            this, SLOT(reactOnClassNameWidgetChange()));
    connect(m_classNameEdit, SIGNAL(clearFocusOut()),
            this, SLOT(reactOnClassNameWidgetChange()));

    connect(m_objectNameEdit, SIGNAL(focusOut()),
            this, SLOT(reactOnObjectNameWidgetChange()));
    connect(m_objectNameEdit, SIGNAL(clearFocusOut()),
            this, SLOT(reactOnObjectNameWidgetChange()));


}

void
FindDirectChildNodeWidget::setClassNameWidget(const QString& className)
{
    if (m_classNameEdit) m_classNameEdit->setText(className);
}

void
FindDirectChildNodeWidget::setObjectNameWidget(const QString& objectName)
{
    if (m_objectNameEdit) m_objectNameEdit->setText(objectName);
}

void
FindDirectChildNodeWidget::updateNameCompleter(const ObjectData* data)
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
FindDirectChildNodeWidget::reactOnClassNameWidgetChange()
{
    if (!m_classNameEdit) return;
    emit updateClass(m_classNameEdit->text());
}

void
FindDirectChildNodeWidget::reactOnObjectNameWidgetChange()
{
    if (!m_objectNameEdit) return;
    emit updateObjectName(m_objectNameEdit->text());
}


void
FindDirectChildNodeWidget::updateClassText()
{
    if (!m_classNameEdit) return;

    auto* prop = qobject_cast<GtStringProperty*>(sender());

    if (!prop) return;

    m_classNameEdit->setText(prop->getVal());
}

void
FindDirectChildNodeWidget::updateNameText()
{
    if (!m_objectNameEdit) return;

    auto* prop = qobject_cast<GtStringProperty*>(sender());

    if (!prop) return;

    m_objectNameEdit->setText(prop->getVal());
}
} // namespace intelli
