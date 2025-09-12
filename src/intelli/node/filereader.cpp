/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/filereader.h>

#include <intelli/data/bytearray.h>
#include <intelli/data/file.h>

using namespace intelli;

FileReaderNode::FileReaderNode() :
    Node("File Reader")
{
    setNodeEvalMode(NodeEvalMode::ExclusiveDetached);

    m_inFile = addInPort({typeId<FileData>(), tr("file")}, Required);
    m_outData = addOutPort({typeId<ByteArrayData>(), tr("data")});
}

void
FileReaderNode::eval()
{
    auto const& fileData = nodeData<FileData>(m_inFile);
    if (!fileData)
    {
        setNodeData(m_outData, nullptr);
        return evalFailed();
    }

    QFileInfo info = fileData->value();
    QFile file(info.filePath());

    if (!file.exists() || !file.open(QFile::ReadOnly))
    {
        setNodeData(m_outData, nullptr);
        return evalFailed();
    }

    setNodeData(m_outData, std::make_shared<ByteArrayData>(file.readAll()));
}
