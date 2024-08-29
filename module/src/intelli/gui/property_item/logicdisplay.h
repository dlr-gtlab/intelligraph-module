/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 20.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_LOGICDISPLAY_H
#define GT_INTELLI_LOGICDISPLAY_H

#include <QLabel>

namespace intelli
{

class LogicDisplayWidget : public QWidget
{
    Q_OBJECT

public:

    explicit LogicDisplayWidget(QWidget* parent = nullptr) :
        LogicDisplayWidget(false, parent)
    {}

    explicit LogicDisplayWidget(bool value, QWidget* parent = nullptr);

    bool value() const;

    void setReadOnly(bool readOnly = true);

    bool readOnly() const;

public slots:

    void toogle();

    void setValue(bool value);

signals:

    void valueChanged(bool newValue);

protected:

    void mousePressEvent(QMouseEvent* e) override;

    void paintEvent(QPaintEvent* e) override;

private:

    bool m_value = false, m_readOnly = false;
};

} // namespace intelli

#endif // GT_INTELLI_LOGICDISPLAY_H
