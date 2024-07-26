/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
 *
 * Created on: 11.7.2024
 * Author: Marius Bröcker (AT-TWK)
 * E-Mail: marius.broecker@dlr.de
 */

#include <intelli/node/filewriter.h>

#include <intelli/data/bytearray.h>
#include <intelli/data/file.h>
#include <intelli/data/bool.h>

#include <gt_finally.h>

using namespace intelli;

FileWriterNode::FileWriterNode() :
    Node("File Writer")
{
    setNodeEvalMode(NodeEvalMode::Exclusive);

    m_inFile = addInPort({typeId<FileData>(), tr("file")}, Required);
    m_inData = addInPort({typeId<ByteArrayData>(), tr("data")}, Required);
    m_outSuccess = addOutPort({typeId<BoolData>(), tr("success")});
}

void
FileWriterNode::eval()
{
    bool success = false;
    auto finally = gt::finally([this, &success](){
        if (!success)
        {
            auto const& fileData = nodeData<FileData>(m_inFile);
            gtWarning().verbose()
                    << tr("Failed to write file at '%1'!")
                       .arg(fileData ? fileData->value().filePath() : "");
        }
        setNodeData(m_outSuccess, std::make_shared<BoolData>(success));
    });

    auto const& fileData = nodeData<FileData>(m_inFile);
    auto const& inData = nodeData<ByteArrayData>(m_inData);
    if (!fileData || !inData) return;

    QFileInfo info = fileData->value();
    QFile file(info.filePath());

    if (!file.open(QFile::Truncate | QFile::WriteOnly)) return;

    success = file.write(inData->value());
}
