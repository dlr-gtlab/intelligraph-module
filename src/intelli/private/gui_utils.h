/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */
#ifndef GT_INTELLI_GUI_UTILS_H
#define GT_INTELLI_GUI_UTILS_H

#include <gt_object.h>
#include <gt_inputdialog.h>
#include <gt_icons.h>
#include <gt_qtutilities.h>
#include <gt_regexp.h>
#include <gt_datamodel.h>

#include <QRegExpValidator>

namespace intelli
{
namespace gui_utils
{
template <typename T>
inline void addNamedChild(GtObject& obj)
{
    GtInputDialog dialog{GtInputDialog::TextInput};
    dialog.setWindowTitle(QObject::tr("Name new Object"));
    dialog.setWindowIcon(gt::gui::icon::rename());
    dialog.setLabelText(QObject::tr("Enter a name for the new object."));

    dialog.setTextValidator(new QRegExpValidator{
        gt::re::onlyLettersAndNumbersAndSpace()
    });

    if (!dialog.exec()) return;

    QString text = dialog.textValue();
    if (text.isEmpty()) return;

    auto child = std::make_unique<T>();
    setObjectName(*child, gt::makeUniqueName(text, obj));

    if (gtDataModel->appendChild(child.get(), &obj).isValid())
    {
        child.release();
    }
}
} // namespace utils

} // namespace intelli
#endif // GT_INTELLI_GUI_UTILS_H
