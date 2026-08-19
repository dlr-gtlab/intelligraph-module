/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_COMMENTSTYLEDIALOG_H
#define GT_INTELLI_COMMENTSTYLEDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QPointer>

class QCheckBox;

namespace intelli
{

/// Pushbutton for displaying colors.
/// Supports displaying a primary and secondary color.
class ColorButton : public QPushButton
{
    Q_OBJECT

    QColor m_primary, m_secondary;

public:

    /// constructor for primary and secondary color
    explicit ColorButton(QColor primary, QColor secondary, QWidget* parent = nullptr);

    /// constructor for primary color
    explicit ColorButton(QColor primary, QWidget* parent = nullptr) :
        ColorButton(primary, QColor{}, parent)
    {}

    /**
     * @brief Setter for the primary color
     * @param color Primary color
     */
    void setPrimaryColor(QColor color);

    /**
     * @brief Returns the primary color
     * @return Primary color
     */
    QColor primaryColor() const;

    /**
     * @brief Setter for the secondary color
     * @param color Secondary color
     */
    void setSecondaryColor(QColor color);

    /**
     * @brief Returns the secondary color
     * @return Secondary color
     */
    QColor secondaryColor() const;

protected:

    /// draw the primary and secondary color
    void paintEvent(QPaintEvent* e) override;
};

class CommentData;
class CommentStyleDialog : public QDialog
{
    Q_OBJECT

public:

    explicit CommentStyleDialog(CommentData& comment);
    ~CommentStyleDialog();

    void setBackgroundColor(QColor const& color);

    void setTextColor(QColor const& color);

private:

    QPointer<CommentData> m_comment;

    QVector<ColorButton*> m_bgColorButtons;
    QVector<ColorButton*> m_textColorButtons;

    ColorButton* m_customBgColorButton;
    ColorButton* m_customTextColorButton;

    QColor m_customTextColor;
    QColor m_customBgColor;

private slots:

    void getCustomBackgroundColor();

    void getCustomTextColor();

signals:

    void customTextColorChanged();

    void customBackgroundColorChanged();

    void textAlignmentChanged();
};

} // namespace intelli

#endif // GT_INTELLI_COMMENTSTYLEDIALOG_H
