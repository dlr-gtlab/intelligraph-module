/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_GUI_GRAPHSELECTIONACTION_H
#define GT_INTELLI_GUI_GRAPHSELECTIONACTION_H

#include <intelli/exports.h>
#include <intelli/globals.h>

#include <QIcon>
#include <QString>
#include <QVector>

#include <functional>

class QObject;
class QWidget;

namespace intelli
{

class Graph;

struct GT_INTELLI_EXPORT GraphSelectionAction
{
    /// A non-empty ID replaces any previous registration with the same ID.
    QString id;
    QString text;
    QIcon icon;
    /// An empty predicate is treated as visible.
    std::function<bool(Graph const&, QVector<ObjectUuid> const&)> isVisible;
    /// An empty predicate is treated as enabled.
    std::function<bool(Graph const&, QVector<ObjectUuid> const&)> isEnabled;
    std::function<void(Graph&, QVector<ObjectUuid> const&, QWidget*)> trigger;
};

/**
 * @brief Registers an action for graph-object selections.
 *
 * The registration remains valid for the lifetime of @p owner. Destroying the
 * owner unregisters the action and invalidates previously returned snapshots.
 * A non-empty action ID replaces an existing registration with the same ID;
 * actions with empty IDs coexist. Registration and owner destruction must take
 * place on the application thread.
 */
GT_INTELLI_EXPORT
void registerGraphSelectionAction(QObject& owner, GraphSelectionAction action);

/**
 * @brief Returns snapshots of all currently registered selection actions.
 *
 * Callback functions in a snapshot become inert when the registration owner
 * is destroyed or its ID is replaced. String data and 16 x 16 icon pixmaps are
 * copied into host-owned storage so snapshots remain safe after module unload.
 * This function and the returned callbacks must be used on the application
 * thread. The built-in context-menu integration passes callbacks a sorted,
 * duplicate-free selection without empty UUIDs.
 */
GT_INTELLI_EXPORT
QVector<GraphSelectionAction> graphSelectionActions();

} // namespace intelli

#endif // GT_INTELLI_GUI_GRAPHSELECTIONACTION_H
