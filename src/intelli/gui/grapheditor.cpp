/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/grapheditor.h"

#include <intelli/graph.h>
#include <intelli/graphexecmodel.h>
#include <intelli/gui/graphscene.h>
#include <intelli/gui/graphscenemanager.h>
#include <intelli/gui/graphstatemanager.h>
#include <intelli/gui/graphview.h>
#include <intelli/gui/graphviewoverlay.h>
#include <intelli/gui/style.h>
#include <intelli/private/utils.h>

#include <gt_logging.h>

#include <gt_application.h>
#include <gt_project.h>
#include <gt_icons.h>

#include <QVBoxLayout>
#include <QPushButton>

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

GraphEditor::~GraphEditor() = default;

void
GraphEditor::setData(GtObject* obj)
{
    assert(m_view);
    assert(m_sceneManager);

    auto* graph  = qobject_cast<Graph*>(obj);
    if (!graph)
    {
        gtError().verbose() << tr("Not an intelli graph!") << obj;
        return;
    }

    // setup exec model
    auto* model = GraphExecutionModel::make(*graph);
    if (!model)
    {
        gtError()
            << tr("Failed to create exec model for graph '%1'!")
                   .arg(relativeNodePath(*graph));
        return;
    }
    model->setScope(gtApp->currentProject());
    model->reset();

    // setup state manager
    GraphStateManager::make(*graph, *m_view);

    // create initial scene
    auto* scene = m_sceneManager->createScene(*graph);
    if (!scene)
    {
        gtError().verbose()
            << tr("Failed to create scene for graph '%1'!")
                   .arg(relativeNodePath(*graph));
        return;
    }

    setObjectName(tr("IntelliGraph Editor") + QStringLiteral(" - ") + graph->caption());

    connect(graph, &QObject::destroyed, this, &QObject::deleteLater);
}

void
GraphEditor::initialized()
{
    assert(!m_view);
    assert(!m_sceneManager);
    assert(!m_overlay);

    m_view = new GraphView;
    m_view->setFrameShape(QFrame::NoFrame);

    auto* l = new QVBoxLayout(widget());
    l->addWidget(m_view);
    l->setContentsMargins(0, 0, 0, 0);

    m_sceneManager = GraphSceneManager::make(*m_view);

    m_overlay = GraphViewOverlay::make(*m_view);
    
    connect(m_overlay, &GraphViewOverlay::sceneChangeRequested,
            m_sceneManager, &GraphSceneManager::openGraphByUuid);
}
