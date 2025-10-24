/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GUIUTILITIES_H
#define GT_INTELLI_GUIUTILITIES_H

#include <intelli/exports.h>
#include <gt_platform.h>

#include <memory>

class QWidget;

namespace intelli
{
namespace utils
{

/**
 * @brief Creates an empty widget that has a simple vertical layout attached.
 * This widget can be used for node widgets, that have trouble resizing
 * correctly, such as `QTextEdit`s. The content's margins of the layout are
 * zeroed.
 * @return Widget pointer (never null). Widget's layout is also not null.
 */
GT_NO_DISCARD
GT_INTELLI_EXPORT
std::unique_ptr<QWidget> makeWidgetWithLayout();

} // namespace

} // namespace intelli

#endif // GT_INTELLI_GUIUTILITIES_H
