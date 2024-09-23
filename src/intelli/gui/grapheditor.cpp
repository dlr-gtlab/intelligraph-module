/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/grapheditor.h"

#include "intelli/gui/graphview.h"
#include "intelli/gui/style.h"

#include <gt_logging.h>

#include <gt_application.h>

#include <QVBoxLayout>

/*
 * generated 1.2.0
 */

using namespace intelli;

GraphEditor::GraphEditor() : m_view(nullptr)
{
    // update theme automatically

    static auto initOnce = gtApp && [](){

        auto const updateStyle = [](bool isDark){

            auto const& currentStyle = style::currentStyle().id;
            bool isDefault =
                (currentStyle == style::styleId(style::DefaultStyle::Bright)) ||
                (currentStyle == style::styleId(style::DefaultStyle::Dark));

            if (isDefault) style::applyStyle(isDark ?
                                      style::DefaultStyle::Dark :
                                      style::DefaultStyle::Bright);
        };

        gtApp->connect(gtApp, &GtApplication::themeChanged,
                       gtApp, updateStyle);
        updateStyle(gtApp->inDarkMode());

        return 0;
    }();
    Q_UNUSED(initOnce);

    setObjectName(tr("IntelliGraph Editor"));
}

void
GraphEditor::setData(GtObject* obj)
{
    assert(m_view);

    auto graph  = qobject_cast<Graph*>(obj);
    if (!graph)
    {
        gtError().verbose() << tr("Not an intelli graph!") << obj;
        return;
    }

    if (m_scene)
    {
        gtError().verbose()
            << tr("Expected null intelli graph scene, aborting!");
        return;
    }

    // instantly commit suicide if widget is destroyed (avoids issue #87)
    connect(graph, &QObject::destroyed, this, [this](){ delete this; });

    // close graph model if its no longer used
    m_scene = make_volatile<GraphScene, DirectDeleter>(*graph);

    m_view->setScene(*m_scene);

    setObjectName(tr("IntelliGraph Editor") + QStringLiteral(" - ") + graph->caption());
}

void
GraphEditor::initialized()
{
    assert(!m_view);
    m_view = new GraphView;
    m_view->setFrameShape(QFrame::NoFrame);

    auto* l = new QVBoxLayout(widget());
    l->addWidget(m_view);
    l->setContentsMargins(0, 0, 0, 0);
}
