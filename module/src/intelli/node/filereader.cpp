/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
 *
 * Created on: 24.6.2024
 * Author: Marius Bröcker (AT-TWK)
 * E-Mail: marius.broecker@dlr.de
 */

#include <intelli/node/filereader.h>

#include <intelli/data/bytearray.h>
#include <intelli/data/file.h>

using namespace intelli;

FileReaderNode::FileReaderNode() :
    Node("File Reader")
{
    setNodeEvalMode(NodeEvalMode::Exclusive);

    m_inFile = addInPort({typeId<FileData>(), tr("file")}, Required);
    m_outData = addOutPort({typeId<ByteArrayData>(), tr("data")});
}

void
FileReaderNode::eval()
{
    auto const& fileData = nodeData<FileData>(m_inFile);
    if (!fileData) return (void)setNodeData(m_outData, nullptr);

    QFileInfo info = fileData->value();
    QFile file(info.filePath());

    if (!file.exists() || !file.open(QFile::ReadOnly)) return (void)setNodeData(m_outData, nullptr);

    setNodeData(m_outData, std::make_shared<ByteArrayData>(file.readAll()));
}
