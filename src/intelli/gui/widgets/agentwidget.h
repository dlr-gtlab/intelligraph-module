#pragma once

#include <intelli/node/agent.h>

#include <QWidget>
#include <QTextEdit>
#include <QToolButton>

#include "markdownwindow.h"

class QLabel;
class QVBoxLayout;

namespace intelli
{

class AgentWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AgentWidget(AgentNode& node, QWidget* parent = nullptr);

private slots:
    void showContext();

private:
    AgentNode& m_node;
    QToolButton* m_button{nullptr};
    QTextEdit* textEdit = nullptr;

};
} // namespace intelli


class MarkdownDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MarkdownDialog(QString& text,QString& in, QWidget* parent = nullptr);
private:

private:
    MarkdownWindow* m_systemWindow;
    MarkdownWindow* m_conversationWindow;
;
};
