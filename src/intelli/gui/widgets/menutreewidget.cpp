/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/widgets/menutreewidget.h"

#include <QKeyEvent>

using namespace intelli;

MenuTreeWidget::MenuTreeWidget(QWidget* parent) :
    QTreeWidget(parent)
{ }

void
MenuTreeWidget::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    {
        const QModelIndex idx = currentIndex();
        if (!idx.isValid()) break;

        // fixes: pressing enter on unselectable item triggers QWidgetAction
        if (!idx.flags().testFlag(Qt::ItemIsSelectable))
        {
            expand(idx);
            event->accept();
            return;
        }
        break;
    }
    default:
        break;
    }
    return QTreeWidget::keyPressEvent(event);
}

