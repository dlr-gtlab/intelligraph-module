/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/textdisplaynodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/utilities.h>
#include <intelli/node/textdisplay.h>

#include <gt_application.h>
#include <gt_codeeditor.h>
#include <gt_jshighlighter.h>
#include <gt_pyhighlighter.h>
#include <gt_xmlhighlighter.h>

#include <QGraphicsWidget>
#include <QLayout>
#include <QSyntaxHighlighter>
#include <QTextDocument>

#include <cassert>

using namespace intelli;

TextDisplayNodeUI::TextDisplayNodeUI() = default;

NodeUI::WidgetFactoryFunction
TextDisplayNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<TextDisplayNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<TextDisplayNode*>(&source);
        if (!node) return nullptr;

        auto b = utils::makeWidgetWithLayout();
        auto* lay = b->layout();

        auto* w = new GtCodeEditor();
        lay->addWidget(w);

        w->setMinimumSize(125, 25);
        w->resize(400, 200);
        w->setReadOnly(true);

        auto const updateHighlighter = [node, w]() {
            auto* document = w->document();
            assert(document);

            auto* highlighter = document->findChild<QSyntaxHighlighter*>();
            if (highlighter) highlighter->deleteLater();

            switch (node->textType())
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
        };

        auto const updateText = [node, w]() {
            w->setPlainText(node->displayText());
        };

        QObject::connect(node, &Node::inputDataRecieved, w, updateText);
        QObject::connect(node,
                         qOverload<GtObject*, GtAbstractProperty*>(&Node::dataChanged),
                         w,
                         updateHighlighter);
        QObject::connect(gtApp, &GtApplication::themeChanged, w, updateHighlighter);

        updateText();
        updateHighlighter();

        return convertToGraphicsWidget(std::move(b), object);
    };
}
