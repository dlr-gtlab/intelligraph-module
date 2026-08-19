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

/// Dialog to edit the style (e.g. colors and text alignemnt) of a comment in
/// real time.
class CommentStyleDialog : public QDialog
{
    Q_OBJECT

public:

    explicit CommentStyleDialog(CommentData& comment);
    ~CommentStyleDialog();

    /**
     * @brief Sets the background color and updates the dialog correctly.
     * @param color Background color
     */
    void setBackgroundColor(QColor const& color);

    /**
     * @brief Sets the text color and updates the dialog correctly.
     * @param color Text color
     */
    void setTextColor(QColor const& color);

private:

    /// pointer to comment object
    QPointer<CommentData> m_comment;

    /// list of buttons for setting the background color
    QVector<ColorButton*> m_bgColorButtons;
    /// list of buttons for setting the text color
    QVector<ColorButton*> m_textColorButtons;
    /// button for setting a custom background color
    ColorButton* m_customBgColorButton;
    /// button for setting a custom text color
    ColorButton* m_customTextColorButton;
    /// custom background color
    QColor m_customTextColor;
    /// custom text color
    QColor m_customBgColor;

private slots:

    /// triggers a color dialog to update the custom background color
    void getCustomBackgroundColor();

    /// triggers a color dialog to update the custom text color
    void getCustomTextColor();
};

} // namespace intelli

#endif // GT_INTELLI_COMMENTSTYLEDIALOG_H
