/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */
#pragma once

#include <intelli/DynamicNode.h>

#include <gt_enumproperty.h>
#include <gt_boolproperty.h>
#include <gt_stringproperty.h>

namespace intelli
{

class AgentNode : public DynamicNode
{
    Q_OBJECT

public:

    enum class LLMType
    {
        Gemma3,
        GPT5
    };

    Q_ENUM(LLMType);

    Q_INVOKABLE

    AgentNode();


    QString conversationText() const;
    QString systemText() const;
    void setConversationText(const QString&);
    void setSystemText(const QString&);

signals:
    void contextTextChanged(const QString&);
    void buttonVisibilityChanged();

private:
    intelli::PortId in;
    intelli::PortId out;

    GtBoolProperty m_hideButton;
    GtEnumProperty<LLMType> m_llmType;
    GtStringProperty m_contextText{"LLMOutput","LLM Output"};
    GtStringProperty m_sysText{"SystemPrompt","System Prompt"};
protected:
    void eval();
};

} // namespace intelli

