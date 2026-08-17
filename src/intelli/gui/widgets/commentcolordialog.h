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

class QCheckBox;

namespace intelli
{

class CommentColorDialog : public QDialog
{
    Q_OBJECT

public:

    CommentColorDialog();
    ~CommentColorDialog();

    void setShowBorder(bool enable);

    QColor textColor() const;

    QColor backgroundColor() const;

    bool showBorder() const;

private:

    struct Impl;

    QCheckBox* m_showBorderCheckBox;

    QColor m_textColor, m_bgColor;

    bool m_showBorder = true;
};

} // namespace intelli

#endif // GT_INTELLI_COMMENTCOLORDIALOG_H
