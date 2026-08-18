/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_COMMENTCOLORDIALOG_H
#define GT_INTELLI_COMMENTCOLORDIALOG_H

#include <QDialog>
#include <QPushButton>

class QCheckBox;

namespace intelli
{

class ColorButton : public QPushButton
{
    Q_OBJECT

    QColor m_primary, m_secondary;

public:

    ColorButton(QColor primary, QColor secondary = {}, QWidget* parent = nullptr);

    void setPrimaryColor(QColor color);

    QColor primaryColor() const;

    void setSecondaryColor(QColor color);

    QColor secondaryColor() const;

protected:

    void paintEvent(QPaintEvent* e) override;
};

class CommentColorDialog : public QDialog
{
    Q_OBJECT

public:

    CommentColorDialog();
    ~CommentColorDialog();

    void setShowFrame(bool enable);

    bool showFrame() const;

    bool isDefaultBackgroundColor() const;

    void setBackgroundColor(QColor const& color);

    QColor backgroundColor() const;

    bool isDefaultTextColor() const;

    void setTextColor(QColor const& color);

    QColor textColor() const;

    void setTextAlignment(Qt::Alignment alignment);

    Qt::Alignment textAlignment() const;

private:

    QVector<ColorButton*> m_bgColorButtons;

    QVector<ColorButton*> m_textColorButtons;

    ColorButton* m_customBgColorButton;
    ColorButton* m_customTextColorButton;

    QCheckBox* m_showFrameCheckBox;

    QColor m_customTextColor;
    QColor m_customBgColor;

    bool m_showBorder = true;

signals:

    void customTextColorChanged();

    void customBackgroundColorChanged();
};

} // namespace intelli

#endif // GT_INTELLI_COMMENTCOLORDIALOG_H
