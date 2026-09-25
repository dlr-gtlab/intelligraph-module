/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_PRIVATE_GRAPHSELECTIONACTIONMENU_H
#define GT_INTELLI_PRIVATE_GRAPHSELECTIONACTIONMENU_H

#include <intelli/exports.h>
#include <intelli/globals.h>

#include <QPointer>
#include <QVector>

class QMenu;
class QWidget;

namespace intelli
{

class Graph;

namespace detail
{

GT_INTELLI_EXPORT
QPointer<QMenu> createGraphSelectionMenu(QWidget* parent);

GT_INTELLI_EXPORT
bool addGraphSelectionActionsToMenu(QMenu& menu,
                                    Graph& graph,
                                    QVector<ObjectUuid> selection);

} // namespace detail
} // namespace intelli

#endif // GT_INTELLI_PRIVATE_GRAPHSELECTIONACTIONMENU_H
