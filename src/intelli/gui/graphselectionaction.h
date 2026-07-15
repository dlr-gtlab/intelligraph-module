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

class QWidget;

namespace intelli
{

class Graph;

struct GT_INTELLI_EXPORT GraphSelectionAction
{
    QString id;
    QString text;
    QIcon icon;
    std::function<bool(Graph const&, QVector<ObjectUuid> const&)> isVisible;
    std::function<bool(Graph const&, QVector<ObjectUuid> const&)> isEnabled;
    std::function<void(Graph&, QVector<ObjectUuid> const&, QWidget*)> trigger;
};

GT_INTELLI_EXPORT
void registerGraphSelectionAction(GraphSelectionAction action);

GT_INTELLI_EXPORT
QVector<GraphSelectionAction> graphSelectionActions();

} // namespace intelli

#endif // GT_INTELLI_GUI_GRAPHSELECTIONACTION_H
