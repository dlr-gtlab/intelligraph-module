/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/graph.h>
#include <intelli/gui/guidata.h>

#include <gt_application.h>
#include <gt_structproperty.h>

#include <gt_stringproperty.h>

using namespace intelli;

GuiData::GuiData(GtObject* parent) :
    GtObject(parent)
{
    setObjectName(tr("__gui_data"));

    auto* localStates = new LocalStateContainer(this);
    localStates->setDefault(true);

    if (!gtApp || !gtApp->devMode()) setFlag(UserHidden);
}

LocalStateContainer*
GuiData::accessLocalStates(Graph& graph)
{
    auto* guiData = graph.findDirectChild<GuiData*>();
    if (!guiData) return {};

    return guiData->findDirectChild<LocalStateContainer*>();
}

char const* S_TYPE_ID = "Collapsed";

LocalStateContainer::LocalStateContainer(GtObject* parent) :
    GtObject(parent),
    m_collapsed("collapsed", tr("Collapsed Nodes"))
{
    setObjectName(tr("local_states"));

    // presence of struct inidcates node is collapsed
    GtPropertyStructDefinition structType{S_TYPE_ID};
    m_collapsed.registerAllowedType(structType);

    registerPropertyStructContainer(m_collapsed);

    connect(&m_collapsed, &GtPropertyStructContainer::entryAdded,
            this, [this](int idx){
                GtPropertyStructInstance& entry = m_collapsed.at(idx);
                m_collapsedMap.insert(idx, entry.ident());
                emit nodeCollapsedChanged(entry.ident(), true);
                assert(m_collapsed.size() == (size_t)m_collapsedMap.size());
            }, Qt::DirectConnection);

    connect(&m_collapsed, &GtPropertyStructContainer::entryRemoved,
            this, [this](int idx){
                NodeUuid nodeUuid = m_collapsedMap.at(idx);
                m_collapsedMap.removeAt(idx);
                emit nodeCollapsedChanged(nodeUuid, false);
                assert(m_collapsed.size() == (size_t)m_collapsedMap.size());
        }, Qt::DirectConnection);
}

void
LocalStateContainer::init()
{
    assert(m_collapsed.size() == (size_t)m_collapsedMap.size());
    for (GtPropertyStructInstance const& e : m_collapsed)
    {
        emit nodeCollapsedChanged(e.ident(), true);
    }
}

void
LocalStateContainer::setNodeCollapsed(NodeUuid const& nodeUuid, bool collapsed)
{
    if (collapsed == isNodeCollapsed(nodeUuid)) return;

    if (collapsed)
    {
        m_collapsed.newEntry(S_TYPE_ID,
                             std::next(m_collapsed.begin(), m_collapsed.size()),
                             nodeUuid);
        assert(m_collapsed.size() == (size_t)m_collapsedMap.size());
        return;
    }

    auto iter = std::find_if(m_collapsed.begin(), m_collapsed.end(),
                             [&nodeUuid](GtPropertyStructInstance const& e){
                                 return e.ident()  == nodeUuid;
                             });
    if (iter == m_collapsed.end()) return;

    m_collapsed.removeEntry(iter);
    assert(m_collapsed.size() == (size_t)m_collapsedMap.size());
}

bool
LocalStateContainer::isNodeCollapsed(NodeUuid const& nodeUuid) const
{
    assert(m_collapsed.size() == (size_t)m_collapsedMap.size());
    return std::find_if(m_collapsed.begin(), m_collapsed.end(),
                        [&nodeUuid](GtPropertyStructInstance const& e){
                            return e.ident()  == nodeUuid;
                        }) != m_collapsed.end();
}

