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
#include <gt_state.h>
#include <gt_grid.h>

#include <QVBoxLayout>

/*
 * generated 1.2.0
 */

using namespace intelli;

GraphEditor::GraphEditor() :
    m_view(new GraphView)
{
    setObjectName(tr("IntelliGraph Editor"));

    m_view->setFrameShape(QFrame::NoFrame);

    auto* l = new QVBoxLayout(widget());
    l->addWidget(m_view);
    l->setContentsMargins(0, 0, 0, 0);
}

void
GraphEditor::setData(GtObject* obj)
{
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

    // close width if graph is destroyed
    connect(graph, &QObject::destroyed, this, &QObject::deleteLater);

    // close graph model if its no longer used
    m_scene = make_volatile<GraphScene>(*graph);

    m_view->setScene(*m_scene);
    m_view->centerScene();

    setObjectName(tr("IntelliGraph Editor") + QStringLiteral(" - ") + graph->caption());
}
