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

#include <gt_application.h>
#include <gt_object.h>
#include <gt_objectui.h>
#include <gt_inputdialog.h>
#include <gt_icons.h>
#include <gt_qtutilities.h>
#include <gt_regexp.h>
#include <gt_datamodel.h>

#include <QRegExpValidator>

#include "intelli/private/utils.h"

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
    utils::setObjectName(*child, gt::makeUniqueName(text, obj));

    if (gtDataModel->appendChild(child.get(), &obj).isValid())
    {
        child.release();
    }
}

/**
 * @brief Attempts to find an Object UI Action by its shortcut
 * @param object Corresponding object
 * @param shortcut Shortcut to search for
 * @return Object UI Action (invalid if no action was found)
 */
inline GtObjectUIAction findUIActionByShortCut(GtObject& object,
                                               QKeySequence const& shortcut)
{
    QList<GtObjectUI*> const& uis = gtApp->objectUI(&object);
    for (GtObjectUI* ui : uis)
    {
        auto const& actions = ui->actions();
        auto iter = std::find_if(actions.begin(),
                                 actions.end(),
                                 [&shortcut, &object](GtObjectUIAction const& action){
            return shortcut == action.shortCut() &&
                   // action should be visible/enabled
                   (!action.visibilityMethod() ||
                    action.visibilityMethod()(nullptr, &object)) &&
                   (!action.verificationMethod() ||
                    action.verificationMethod()(nullptr, &object));
        });

        if (iter != actions.end()) return *iter;
    }
    return {};
}

} // namespace utils

} // namespace intelli
#endif // GT_INTELLI_GUI_UTILS_H
