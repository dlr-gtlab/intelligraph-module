/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
 *
 * Created on: 24.6.2024
 * Author: Marius Bröcker (AT-TWK)
 * E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_FILEDATA_H
#define GT_INTELLI_FILEDATA_H

#include <intelli/nodedata.h>

#include <QFileInfo>

namespace intelli
{

class GT_INTELLI_EXPORT FileData : public NodeData
{
    Q_OBJECT

public:

    Q_INVOKABLE FileData(QFileInfo file = {});

    Q_INVOKABLE QFileInfo value() const;

private:
    QFileInfo m_file;
};

} // namespace intelli

#endif // GT_INTELLI_FILEDATA_H
