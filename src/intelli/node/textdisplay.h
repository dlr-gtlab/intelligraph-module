/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
 *
 * Created on: 17.6.2024
 * Author: Marius Bröcker (AT-TWK)
 * E-Mail: marius.broecker@dlr.de
 */

#ifndef TEXTDISPLAYNODE_H
#define TEXTDISPLAYNODE_H

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

#endif // TEXTDISPLAYNODE_H
