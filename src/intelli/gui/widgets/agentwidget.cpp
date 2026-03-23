/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Robert Marcenko <robert.schmeink@dlr.de>
 */
#include "agentwidget.h"
#include <intelli/gui/graphics/commentobject.h>
#include <intelli/node/agent.h>

#include <QApplication>

#include <gt_icons.h>

using namespace intelli;

AgentWidget::AgentWidget(AgentNode &node, QWidget* parent) :
    QWidget(parent),
    m_node(node)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_button = new QToolButton(this);
    m_button->setFixedSize(60,60);
    m_button->setIconSize(QSize(50, 50));

    m_button->setToolTip(tr("Select subshapes"));
    layout->addWidget(m_button);
    m_button->setIcon(gt::gui::getIcon(":/intelligraph-icons/ai-robot-icon.svg"));

    connect(m_button, &QToolButton::clicked, this,
                     &AgentWidget::showContext);
}


void AgentWidget::showContext()
{;
    QString convText = m_node.conversationText();
    QString systemText = m_node.systemText();

    MarkdownDialog dialog(convText,systemText, QApplication::activeWindow());
    if (dialog.exec() == QDialog::Rejected)
    {
        m_node.setConversationText(convText);
        m_node.setSystemText(systemText);
    }
}

MarkdownDialog::MarkdownDialog(QString &convText, QString &systemText, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Agent Internals"));
    resize(500, 400);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("<b>System Prompt:</b>"));
    m_systemWindow=new MarkdownWindow(systemText);
    layout->addWidget(m_systemWindow);

    layout->addWidget(new QLabel("<b>Conversation:</b>"));
    m_conversationWindow=new MarkdownWindow(convText);
    layout->addWidget(m_conversationWindow);


}

