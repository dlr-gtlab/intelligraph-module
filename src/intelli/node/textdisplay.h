/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_TEXTDISPLAYNODE_H
#define GT_INTELLI_TEXTDISPLAYNODE_H

#include <intelli/node.h>

#include <gt_enumproperty.h>

namespace intelli
{

class TextDisplayNode : public Node
{
    Q_OBJECT

public:

    /// Enum to differentiate between code highlightings
    enum class TextType
    {
        PlainText,
        Xml,
        Python,
        JavaScript,
    };
    Q_ENUM(TextType);

    Q_INVOKABLE TextDisplayNode();

private:

    GtEnumProperty<TextType> m_textType;
};

} // namespace intelli

#endif // GT_INTELLI_TEXTDISPLAYNODE_H
