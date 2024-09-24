/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/projectinfo.h>

#include <intelli/data/string.h>

#include <gt_coreapplication.h>
#include <gt_project.h>

using namespace intelli;

ProjectInfoNode::ProjectInfoNode() :
    Node("Project Info")
{
    setNodeEvalMode(NodeEvalMode::Blocking);

    m_outName = addOutPort({typeId<StringData>(), tr("name")});
    m_outPath = addOutPort({typeId<StringData>(), tr("path")});
}

void
ProjectInfoNode::eval()
{
    GtProject* project = gtApp->currentProject();

    if (!project)
    {
        for (PortId port : {m_outName, m_outPath}) setNodeData(port, nullptr);
        return;
    }

    setNodeData(m_outName, std::make_shared<StringData>(project->objectName()));
    setNodeData(m_outPath, std::make_shared<StringData>(project->path()));
}
