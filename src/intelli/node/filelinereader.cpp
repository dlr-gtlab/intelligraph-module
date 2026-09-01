/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/node/filelinereader.h"

#include <intelli/data/string.h>
#include <intelli/data/stringlist.h>
#include <intelli/data/list.h>
#include <intelli/data/file.h>

#include <QRegularExpression>

using namespace intelli;

FileLineReaderNode::FileLineReaderNode() :
    Node("File Line Reader")
{
    setNodeEvalMode(NodeEvalMode::ExclusiveDetached);

    m_inFile = addInPort({typeId<FileData>(), tr("file")}, Required);
    m_outData = addOutPort({typeId<StringListData>(), tr("lines")});
}

void
FileLineReaderNode::eval()
{
    Ptr<FileData> const fileData = nodeData<FileData>(m_inFile);
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

    QByteArray content = file.readAll();
    QString decoded = QString::fromLocal8Bit(content);
    QStringList lines = decoded.split(QRegularExpression{QStringLiteral("\\n\\r?")});

    setNodeData(m_outData, std::make_shared<StringListData>(lines));
}
