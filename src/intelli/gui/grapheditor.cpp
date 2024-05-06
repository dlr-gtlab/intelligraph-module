/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 * 
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email: 
 */


#include "intelli/gui/grapheditor.h"

#include "intelli/gui/graphview.h"

#include <gt_logging.h>

#include <QVBoxLayout>

/*
 * generated 1.2.0
 */

using namespace intelli;

GraphEditor::GraphEditor() : m_view(nullptr)
{
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
