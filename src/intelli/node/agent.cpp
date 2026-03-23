/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/agent.h>
#include <intelli/gui/widgets/agentwidget.h>

#include <intelli/data/string.h>
#include <intelli/data/bytearray.h>

#include <gt_application.h>
#include <gt_codeeditor.h>

#include <QLayout>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <qplaintextedit.h>

using namespace intelli;

AgentNode::AgentNode() :
    DynamicNode(tr("Agent Display")),
    m_llmType("llmType", tr("LLM Type"), tr("LLM Type"), LLMType::Gemma3),
    m_hideButton("buttonvisiblity", tr("Hide Button"),tr("Hide the button on the node"),false)
{
    registerProperty(m_llmType);
    registerProperty(m_contextText);
    registerProperty(m_hideButton);
    registerProperty(m_sysText);
    m_contextText.hide();
    m_sysText.hide();

    connect(&m_hideButton, &GtAbstractProperty::changed, this, [this](){
        emit buttonVisibilityChanged();
    });

    QObject::connect(&m_contextText, &GtStringProperty::changed, this, [this](){
        emit contextTextChanged(m_contextText.value<QString>());
    });
    setNodeEvalMode(NodeEvalMode::Blocking);
    setNodeFlag(Resizable, true);

    //Ports
    in = addInPort(makePort(typeId<StringData>())
                              .setCaptionVisible(false));
    out = addOutPort(makePort(typeId<StringData>())
                              .setCaptionVisible(false));
    registerWidgetFactory([=](){
        auto base = makeBaseWidget();
        auto* w = new AgentWidget(*this, base.get());
        base->layout()->addWidget(w);
        w->setMinimumSize(1, 1);
        w->resize(40, 20);
        w->setVisible(!m_hideButton);

        connect(this, &AgentNode::buttonVisibilityChanged,w, [this,w](){
            w->setVisible(!m_hideButton);
        });

        return base;
    });
}

QString
AgentNode::conversationText() const
{
    return m_contextText;
}

QString AgentNode::systemText() const
{
    return m_sysText;
}

// QString AgentNode::getInput() const
// {
//     auto stringData = nodeData<StringData>(in);
//     if (stringData==nullptr){return "";}
//     return stringData->value();
// }

void
AgentNode::setConversationText(const QString & text)
{
    if (text == m_contextText) return;

    m_contextText = text;
    triggerNodeEvaluation();
}

void AgentNode::setSystemText(const QString & text)
{
    if (text == m_sysText) return;

    m_sysText = text;
    triggerNodeEvaluation();
}

void AgentNode::eval(){
        setNodeData(out,std::make_shared<intelli::StringData>(m_contextText.getVal()));
}

