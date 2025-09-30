/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHUSERVARIABLESDIALOG_H
#define GT_INTELLI_GRAPHUSERVARIABLESDIALOG_H

#include <QDialog>
#include <QPointer>

class QListWidget;

namespace intelli
{

class Graph;
class GraphUserVariablesDialog : public QDialog
{
    Q_OBJECT

public:

    GraphUserVariablesDialog(Graph& graph);
    ~GraphUserVariablesDialog();

public slots:

    /**
     * @brief slot for saving all settings
     */
    void saveChanges();

private:

    QListWidget* m_listView{};
    QPointer<Graph> m_graph;

    void load();

    void addItem(QString key, QVariant value);
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHUSERVARIABLESDIALOG_H
