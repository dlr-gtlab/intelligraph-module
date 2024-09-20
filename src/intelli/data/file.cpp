/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/data/file.h>

using namespace intelli;

FileData::FileData(QFileInfo file) :
    NodeData(QStringLiteral("file")),
    m_file(file)
{

}

QFileInfo
FileData::value() const
{
    return m_file;
}

