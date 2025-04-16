/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_ABSTRACTNUMBERINPUTWIDGET_H
#define GT_INTELLI_ABSTRACTNUMBERINPUTWIDGET_H

#include <QWidget>
#include <QVariant>

class QDial;
class QSlider;
class QLabel;
class GtLineEdit;

namespace intelli
{

class EditableLabel;

class AbstractNumberInputWidget : public QWidget
{
    Q_OBJECT

public:

    /// Input Type
    enum InputMode
    {
        LineEditUnbound = 0, // LineEdit that does not enforce bounds
        LineEditBound = 1, // LineEdit that enforces bounds
        SliderV = 2, // Vertical slider, enforces bounds
        SliderH = 3, // Horizontal slider, enforces bounds
        Dial = 4,    // dial/knob, enforces bounds
    };
    Q_ENUM(InputMode)

    /**
     * @brief Updates the current display mode including input mask configuration
     * @param mode Display mode
     */
    void setInputMode(InputMode mode);
    InputMode inputMode() const;

    /// Whether bounds should be enforced.
    bool useBounds() const;

    /// Accesses the current value as a type `T`
    template <typename T>
    T value() const { return QVariant(value()).value<T>(); }

    /// Accesses the current value in underlying string format
    QString value() const;

    void setRange(QVariant const& valueV,
                  QVariant const& minV,
                  QVariant const& maxV);

    template <typename T>
    void setRange(T value, T min, T max)
    {
        return setRange(QVariant::fromValue(value),
                        QVariant::fromValue(min),
                        QVariant::fromValue(max));
    }

    bool eventFilter(QObject* obj, QEvent* e) override;

    QSlider* slider();
    QSlider const* slider() const;

signals:

    /// emitted if value changes, this value may not be the final value
    /// (e.g. emitted while value is edited)
    void valueChanged();

    /// emitted once value has been edited (=final)
    void valueComitted();

    /// whether min bounds changed
    void minChanged();

    /// whether max bounds changed
    void maxChanged();

protected:

    AbstractNumberInputWidget(InputMode mode,
                              EditableLabel* low,
                              EditableLabel* high,
                              QWidget* parent = nullptr);

    GtLineEdit* valueEdit();
    GtLineEdit const* valueEdit() const;

    EditableLabel* low();
    EditableLabel const* low() const;

    EditableLabel* high();
    EditableLabel const* high() const;

    QDial* dial();
    QDial const* dial() const;



protected slots:

    virtual void applyRange(QVariant const& valueV,
                            QVariant const& minV,
                            QVariant const& maxV) = 0;

    virtual void commitSliderValueChange(int value) = 0;

    virtual void commitMinValueChange() = 0;

    virtual void commitMaxValueChange() = 0;

    virtual void commitValueChange() = 0;

private:

    InputMode m_mode = LineEditUnbound;

    QDial* m_dial{nullptr};
    QSlider* m_slider{nullptr};

    GtLineEdit* m_text{nullptr};

    EditableLabel* m_low{nullptr};

    EditableLabel* m_high{nullptr};

    bool m_useBounds{false};

    void applyInputMode(InputMode mode);

private slots:

    void onValueEdited();

    void onMinEdited();

    void onMaxEdited();
};

} // namespace intelli

#endif // GT_INTELLI_ABSTRACTNUMBERINPUTWIDGET_H
