/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/ui/commentui.h>
#include <intelli/gui/commentgroup.h>

#include <gt_icons.h>

using namespace intelli;

CommentUI::CommentUI() = default;

QIcon
CommentUI::icon(GtObject* obj) const
{
    if (qobject_cast<CommentGroup*>(obj))
    {
        return gt::gui::icon::data();
    }
    return gt::gui::icon::comment();
}
