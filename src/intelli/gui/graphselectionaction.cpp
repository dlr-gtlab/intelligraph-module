/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include "intelli/gui/graphselectionaction.h"

#include <utility>

using namespace intelli;

namespace
{

QVector<GraphSelectionAction>&
registeredActions()
{
    static QVector<GraphSelectionAction> actions;
    return actions;
}

} // namespace

void
intelli::registerGraphSelectionAction(GraphSelectionAction action)
{
    auto& actions = registeredActions();
    if (!action.id.isEmpty())
    {
        for (GraphSelectionAction& existing : actions)
        {
            if (existing.id == action.id)
            {
                existing = std::move(action);
                return;
            }
        }
    }

    actions.push_back(std::move(action));
}

QVector<GraphSelectionAction>
intelli::graphSelectionActions()
{
    return registeredActions();
}
