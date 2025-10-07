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
class QCheckBox;
class QLineEdit;
class QComboBox;
class QPushButton;

namespace intelli
{

class EditableLabel;
class Graph;
class GraphUserVariablesDialog : public QDialog
{
    Q_OBJECT

public:

    explicit GraphUserVariablesDialog(Graph& graph);
    ~GraphUserVariablesDialog();

    bool validate() const;

public slots:

    void open() override;
    /**
     * @brief slot for saving all settings
     */
    void saveChanges();

private:

    QPointer<Graph> m_graph;
    QListWidget* m_listView{};
    QPushButton* m_saveButton{};

    void load();

    void addItem(QString key, QVariant value);
};

class GraphUserVariableItem : public QWidget
{
    Q_OBJECT

public:

    GraphUserVariableItem(QString key, QVariant value, QWidget* parent = nullptr);

    bool isActivated() const;

    QString key() const;

    QVariant value() const;

    bool isValid() const;

    void setIsDuplicateKey(bool isDuplicate);

    void init();

signals:

    void keyChanged();

    void valueChanged();

private:

    QCheckBox* m_enableCheckBox{};
    EditableLabel* m_keyLabel{};
    QLineEdit* m_valueEdit{};
    QComboBox* m_typeComboBox{};

private slots:

    void onValueChanged();
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHUSERVARIABLESDIALOG_H
