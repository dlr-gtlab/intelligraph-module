/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 27.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#ifndef EDITABLEDOUBLELABEL_H
#define EDITABLEDOUBLELABEL_H

#include <QStackedWidget>

class QLabel;
class QLineEdit;

namespace intelli
{

class EditableDoubleLabel : public QStackedWidget
{  
    Q_OBJECT
public:
    Q_INVOKABLE EditableDoubleLabel(QString const& text,
                                    QWidget* parent = nullptr);

    double value() const;

    void setValue(const double& value, bool emmit = true);

    bool eventFilter(QObject* watched, QEvent* event) override;

    QFont labelFont();

    void setLabelFont(QFont const& f);

    void setTextAlignment(Qt::Alignment textAlignment);

signals:
    void valueChanged(const double& newVal);

private slots:
    void onTextChanged();

private:
    QLabel* m_l;

    QLineEdit* m_e;
};

} /// namespace intelli
#endif // EDITABLEDOUBLELABEL_H
