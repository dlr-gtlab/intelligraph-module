/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_EDITABLEINTEGERLABEL_H
#define GT_INTELLI_EDITABLEINTEGERLABEL_H

#include <QStackedWidget>

class QLabel;
class QLineEdit;

namespace intelli
{
class EditableIntegerLabel : public QStackedWidget
{  
    Q_OBJECT
public:
    Q_INVOKABLE explicit EditableIntegerLabel(QString const& text,
                                              QWidget* parent = nullptr);

    int value() const;

    void setValue(const int& value, bool emmit = true);

    bool eventFilter(QObject* watched, QEvent* event) override;

    QFont labelFont();

    void setLabelFont(QFont const& f);

    void setTextAlignment(Qt::Alignment textAlignment);

signals:
    void valueChanged(const int& newVal);

private slots:
    void onTextChanged();

private:
    QLabel* m_l{nullptr};

    QLineEdit* m_e{nullptr};
};

} // namespace intelli

#endif // GT_INTELLI_EDITABLEINTEGERLABEL_H
