/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/graphsceneselector.h>

#include <intelli/graph.h>

#include <QLabel>
#include <QVBoxLayout>

using namespace intelli;

GraphSceneSelector::GraphSceneSelector(QWidget* parent) :
    QWidget(parent)
{
    assert(!layout());

    m_scenePath = new QLabel;
    m_scenePath->setTextFormat(Qt::RichText);
    m_scenePath->setTextInteractionFlags(Qt::LinksAccessibleByMouse);

    connect(this, &GraphSceneSelector::graphSelected,
            this, [](QString const& link){
        gtDebug() << "CLICKED:" << link;
    });
    connect(m_scenePath, &QLabel::linkActivated,
            this, &GraphSceneSelector::graphSelected);

    auto* lay = new QVBoxLayout(this);
    lay->addWidget(m_scenePath);
    lay->setContentsMargins(0, 0, 0, 0);
}

void
GraphSceneSelector::setCurrentGraph(Graph& graph)
{
    m_currentGraph = graph.rootGraph();
    assert(m_currentGraph);

    refresh();
}

void
GraphSceneSelector::clear()
{
    m_scenePath->clear();
}

void
GraphSceneSelector::refresh()
{
    clear();

    if (!m_currentGraph) return;

    QString text;

    Graph* graph = m_currentGraph;
    while (graph)
    {
        graph->disconnect(this);

        connect(graph, &QObject::destroyed,
                this, &GraphSceneSelector::refresh);
        connect(graph, &QObject::objectNameChanged,
                this, &GraphSceneSelector::refresh);

        if (!text.isEmpty()) text.push_front(QStringLiteral(" / "));

        text.push_front(QStringLiteral("<a href=\"%1\">%2</a>")
                            .arg(graph->uuid(), graph->caption()));

        graph = graph->parentGraph();
    }

    m_scenePath->setText(std::move(text));
}
