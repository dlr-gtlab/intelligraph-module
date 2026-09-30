/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/textdisplaynodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/node/textdisplay.h>

#include <gt_application.h>
#include <gt_codeeditor.h>
#include <gt_jshighlighter.h>
#include <gt_pyhighlighter.h>
#include <gt_xmlhighlighter.h>

#include <QGraphicsWidget>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QVBoxLayout>

#include <cassert>

using namespace intelli;

TextDisplayNodeWidget::TextDisplayNodeWidget(TextDisplayNode& node, QWidget* parent) :
    QWidget(parent)
{
    m_node = &node;

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    m_editor = new GtCodeEditor(this);
    lay->addWidget(m_editor);

    m_editor->setMinimumSize(125, 25);
    m_editor->resize(400, 200);
    m_editor->setReadOnly(true);

    QObject::connect(m_node, &Node::inputDataRecieved,
                     this, &TextDisplayNodeWidget::updateTextFromNode);
    QObject::connect(m_node,
                     qOverload<GtObject*, GtAbstractProperty*>(&Node::dataChanged),
                     this,
                     &TextDisplayNodeWidget::updateHighlighterFromNode);
    QObject::connect(gtApp, &GtApplication::themeChanged,
                     this, &TextDisplayNodeWidget::updateHighlighterFromNode);

    updateTextFromNode();
    updateHighlighterFromNode();
}

NodeUI::QGraphicsWidgetPtr
TextDisplayNodeWidget::create(Node& source, NodeGraphicsObject& object)
{
    auto* node = qobject_cast<TextDisplayNode*>(&source);
    if (!node) return nullptr;

    auto w = std::make_unique<TextDisplayNodeWidget>(*node);
    return NodeUI::convertToGraphicsWidget(std::move(w), object);
}

void
TextDisplayNodeWidget::updateTextFromNode()
{
    m_editor->setPlainText(m_node->displayText());
}

void
TextDisplayNodeWidget::updateHighlighterFromNode()
{
    auto* document = m_editor->document();
    assert(document);

    auto* highlighter = document->findChild<QSyntaxHighlighter*>();
    if (highlighter) highlighter->deleteLater();

    switch (m_node->textType())
    {
    case TextDisplayNode::TextType::PlainText:
        break;
    case TextDisplayNode::TextType::Xml:
        new GtXmlHighlighter(document);
        break;
    case TextDisplayNode::TextType::Python:
        new GtPyHighlighter(document);
        break;
    case TextDisplayNode::TextType::JavaScript:
        new GtJsHighlighter(document);
        break;
    }
}

TextDisplayNodeUI::TextDisplayNodeUI() = default;

NodeUI::WidgetFactoryFunction
TextDisplayNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<TextDisplayNode const*>(&n)) return {};

    return &TextDisplayNodeWidget::create;
}
