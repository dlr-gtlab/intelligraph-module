/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/input/fileinput.h>

#include <intelli/data/string.h>
#include <intelli/data/file.h>

#include <gt_coreapplication.h>

#include <QDir>

using namespace intelli;

FileInputNode::FileInputNode() :
    Node("File Input"),
    m_fileChooser("file", tr("File"), tr("File Path"), QStringList{})
{
    setNodeFlag(ResizableHOnly, true);

    registerProperty(m_fileChooser);

    m_inDir = addInPort({typeId<StringData>(), tr("dir_path")});
    m_inName = addInPort({typeId<StringData>(), tr("file_name")});
    m_outFile = addOutPort({typeId<FileData>(), tr("file")});

    connect(&m_fileChooser, &GtAbstractProperty::changed, this, [this]() {
        emit selectedFileChanged(selectedFile());
        emit triggerNodeEvaluation();
    });

    auto const updateFileNameConnection = [this](PortId portId, bool connected) {
        if (portId != m_inName) return;
        setNodeFlag(ResizableHOnly, !connected);
        emit nodeChanged();
        emit fileNameInputConnectionChanged(connected);
    };

    connect(this, &Node::portConnected, this, [updateFileNameConnection](PortId portId) {
        updateFileNameConnection(portId, true);
    });
    connect(this, &Node::portDisconnected, this, [updateFileNameConnection](PortId portId) {
        updateFileNameConnection(portId, false);
    });
}

bool
FileInputNode::isFileNameInputConnected() const
{
    auto const* p = port(m_inName);
    return p && p->isConnected();
}

QString
FileInputNode::selectedFile() const
{
    return m_fileChooser.get();
}

QString
FileInputNode::dialogDirectory() const
{
    if (auto const& dirData = nodeData<StringData>(m_inDir))
    {
        return dirData->value();
    }
    return {};
}

void
FileInputNode::setSelectedFile(QString const& filePath)
{
    if (filePath == selectedFile()) return;

    auto cmd = gtApp->makeCommand(this, tr("File Input changed"));
    Q_UNUSED(cmd);
    m_fileChooser.setVal(filePath);
}

void
FileInputNode::eval()
{
    QString const& filePath = selectedFile();

    auto const& nameData = nodeData<StringData>(m_inName);

    QFileInfo fileInfo{filePath};

    if (nameData)
    {
        auto const& dirData = nodeData<StringData>(m_inDir);

        QDir dir;
        if (dirData) dir.setPath(dirData->value());

        fileInfo = QFileInfo{dir, nameData->value()};
    }

    setNodeData(m_outFile, std::make_shared<FileData>(fileInfo));
}
