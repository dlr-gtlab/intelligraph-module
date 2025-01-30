/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_EDITABLELABEL_H
#define GT_INTELLI_EDITABLELABEL_H

#include <QStackedWidget>
#include <QVariant>

class QLabel;
class QLineEdit;

namespace intelli
{

class EditableBaseLabel : public QStackedWidget
{
    Q_OBJECT

public:

    Q_INVOKABLE explicit EditableBaseLabel(QString const& text = {},
                                           QWidget* parent = nullptr);

    QString text() const;

    void setText(QString const& text, bool emitSignal = true);

    void setReadOnly(bool value);
    bool readOnly() const;

    bool eventFilter(QObject* watched, QEvent* event) override;

    void setTextAlignment(Qt::Alignment textAlignment);

    QLabel* label();
    QLabel const* label() const;

    QLineEdit* edit();
    QLineEdit const* edit() const;

    template <typename T>
    inline T value() const
    {
        // hack to convert a string to any `T`
        return QVariant(text()).value<T>();
    }

    template <typename T>
    inline void setValue(T const& value, bool emitSignal = true)
    {
        // hack to convert a most `T` to strings
        return setText(QStringLiteral("%1").arg(value), emitSignal);
    }

signals:

    void textChanged();

private slots:

    void onTextEdited();

private:

    QLabel* m_label{nullptr};
    QLineEdit* m_edit{nullptr};
    bool m_readOnly{false};
};

template <typename T>
class EditableLabel : public EditableBaseLabel
{
public:

    explicit EditableLabel(QString const& text = {}, QWidget* parent = nullptr) :
        EditableBaseLabel(text, parent)
    { }

    inline T value() const { return EditableBaseLabel::value<T>(); }

    inline void setValue(T const& value, bool emitSignal = true)
    {
        return EditableBaseLabel::setValue<T>(value, emitSignal);
    }
};

class EditableIntegerLabel : public EditableLabel<int>
{
    Q_OBJECT
public:
    explicit EditableIntegerLabel(QString const& text = {}, QWidget* parent = nullptr);
};

class EditableDoubleLabel : public EditableLabel<double>
{
    Q_OBJECT
public:
    explicit EditableDoubleLabel(QString const& text = {}, QWidget* parent = nullptr);
};

} // namespace intelli

#endif // GT_INTELLI_EDITABLELABEL_H
