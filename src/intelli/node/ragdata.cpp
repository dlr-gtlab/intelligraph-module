/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/ragdata.h>

#include <intelli/data/string.h>
#include <intelli/data/bytearray.h>

#include <gt_application.h>
#include <gt_codeeditor.h>

#include <QLayout>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <qplaintextedit.h>
#include <QStackedLayout>
#include <QApplication>

using namespace intelli;

RagDataNode::RagDataNode():
    DynamicNode("RAG Data")
{
    registerProperty(m_hideButton);
    registerProperty(m_pathToImage);
    registerProperty(m_text);
    registerProperty(m_text1);
    registerProperty(m_text2);
    m_text.hide();
    m_text1.hide();
    m_text2.hide();

    setNodeFlag(Resizable, true);

    registerWidgetFactory([=](){
        auto base = makeBaseWidget();
        auto* w = new RagWidget(*this, base.get());
        base->layout()->addWidget(w);

        w->setVisible(!m_hideButton);

        w->setMinimumSize(1, 1);
        w->resize(40, 20);

        connect(&m_hideButton, &GtAbstractProperty::changed,w, [this,w](){
            w->setVisible(!m_hideButton);
        });
        return base;
    });
}

QString RagDataNode::text(int i) const
{
    switch (i)
    {
    case 0: return m_text;
    case 1: return m_text1;
    case 2: return m_text2;
    default: return QString();
    }
}


void
RagDataNode::setText(QString text,int i)
{
    switch (i)
    {
    case 0: m_text.setVal(text); break;
    case 1: m_text1.setVal(text); break;
    case 2: m_text2.setVal(text); break;
    }
    triggerNodeEvaluation();
}

RAGDialog::RAGDialog(RagDataNode& node, QWidget* parent)
    : QDialog(parent),
    m_node(node),
    m_text{m_node.text(0), m_node.text(1), m_node.text(2)}
{
    setWindowTitle("RAG Data");
    resize(750, 400);

    auto* mainlayout = new QHBoxLayout(this);

    auto* leftlayout = new QVBoxLayout(this);
    leftlayout->addWidget(new QLabel("<b>(1)</b>"));
    m_window=new MarkdownWindow(m_text[0]);
    leftlayout->addWidget(m_window);

    leftlayout->addWidget(new QLabel("<b>(2)</b>"));
    m_window1=new MarkdownWindow(m_text[1]);
    leftlayout->addWidget(m_window1);

    QString style = QString(
        "QTextEdit {"
        " color: white;"
        " background-color: #25445e;"
        " border: 1px solid #c5860d;"
        "}"
        );
    m_window->setStyleSheet(style);
    m_window1->setStyleSheet(style);

    leftlayout->addWidget(new QLabel("<b>(3)</b>"));
    leftlayout->addWidget(new MarkdownWindow(m_text[2]));

    mainlayout->addLayout(leftlayout);

    QLabel* imageLabel = new QLabel();
    QPixmap pm(m_node.m_pathToImage);
    imageLabel->setPixmap(pm);
    imageLabel->setScaledContents(true);

    mainlayout->addWidget(imageLabel);
}

void RAGDialog::saveText()
{
    m_node.setText(m_text[0],0);
    m_node.setText(m_text[1],1);
    m_node.setText(m_text[2],2);
}


RagWidget::RagWidget(RagDataNode &node, QWidget *parent):
    QWidget(parent),
    m_node(node)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    Button = new QToolButton();
    Button->setFixedSize(60,60);
    Button->setIconSize(QSize(50, 50));

    Button->setToolTip(tr("Select subshapes"));
    layout->addWidget(Button);
    Button->setIcon(gt::gui::icon::collection());

    connect(Button, &QToolButton::clicked, this,
            &RagWidget::showContext);
}

void RagWidget::showContext(){
    RAGDialog dialog(m_node, QApplication::activeWindow());
    if (dialog.exec() == QDialog::Rejected)
    {
        dialog.saveText();
    }
}
