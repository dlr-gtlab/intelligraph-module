/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
 *
 * Created on: 24.6.2024
 * Author: Marius Bröcker (AT-TWK)
 * E-Mail: marius.broecker@dlr.de
 */

#include <intelli/data/file.h>

using namespace intelli;

FileData::FileData(QFileInfo file) :
    NodeData("file"),
    m_file(file)
{

}

QFileInfo
FileData::value() const
{
    return m_file;
}

