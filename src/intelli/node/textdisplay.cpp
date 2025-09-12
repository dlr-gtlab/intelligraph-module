/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/textdisplay.h>

#include <intelli/data/string.h>
#include <intelli/data/bytearray.h>

#include <gt_application.h>
#include <gt_xmlhighlighter.h>
#include <gt_pyhighlighter.h>
#include <gt_jshighlighter.h>
#include <gt_xmlhighlighter.h>
#include <gt_codeeditor.h>

#include <QLayout>
#include <QSyntaxHighlighter>
#include <QTextDocument>

using namespace intelli;

TextDisplayNode::TextDisplayNode() :
    Node("Text Display"),
    m_textType("textType", tr("Text Type"), tr("Text Type"), TextType::PlainText)
{
    registerProperty(m_textType);

    setNodeEvalMode(NodeEvalMode::Blocking);
    setNodeFlag(Resizable, true);

    PortId in = addInPort(makePort(typeId<StringData>())
                              .setCaptionVisible(false));

    registerWidgetFactory([this, in](){
        auto base = makeBaseWidget();

        auto* w = new GtCodeEditor();
        base->layout()->addWidget(w);

        w->setMinimumSize(125, 25);
        w->resize(400, 200);
        w->setReadOnly(true);

        // update sytnax highlighter
        auto const updateHighlighter = [this, w](){
            auto* document = w->document();
            assert(document);

            auto* highlighter = document->findChild<QSyntaxHighlighter*>();
            if (highlighter) highlighter->deleteLater();

            switch (m_textType.getVal())
            {
            case TextType::PlainText:
                break;
            case TextType::Xml:
                new GtXmlHighlighter(document);
                break;
            case TextType::Python:
                new GtPyHighlighter(document);
                break;
            case TextType::JavaScript:
                new GtJsHighlighter(document);
                break;
            }
        };

        // update text
        auto const updateText = [this, in, w](){
            w->clear();
            if (auto const& data = nodeData<StringData>(in))
            {
                w->setPlainText(data->value());
            }
        };

        connect(this, &Node::inputDataRecieved,
                w, updateText);
        connect(this, qOverload<GtObject*, GtAbstractProperty*>(&Node::dataChanged),
                w, updateHighlighter);
        connect(gtApp, &GtApplication::themeChanged,
                w, updateHighlighter);

        updateText();
        updateHighlighter();

        return base;
    });
}
