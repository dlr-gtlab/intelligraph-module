/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_MENUTREEWIDGET_H
#define GT_INTELLI_MENUTREEWIDGET_H

#include <QTreeWidget>

namespace intelli
{

} // namespace intelli

class MenuTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    MenuTreeWidget(QWidget* parent);

protected:

    void keyPressEvent(QKeyEvent* event) override;
};

#endif // GT_INTELLI_MENUTREEWIDGET_H
