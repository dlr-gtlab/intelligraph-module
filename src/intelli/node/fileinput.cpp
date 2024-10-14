/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/fileinput.h>

#include <intelli/data/string.h>
#include <intelli/data/file.h>

#include <gt_coreapplication.h>
#include <gt_propertyfilechoosereditor.h>
#include <gt_filedialog.h>

#include <QDir>
#include <QLayout>
#include <QPushButton>
#include <QFileDialog>

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

    registerWidgetFactory([this](){
        auto b = makeBaseWidget();
        auto* lay = b->layout();

        auto w = new GtPropertyFileChooserEditor;
        lay->addWidget(w);
        w->setFileChooserProperty(&m_fileChooser);

        // show/hide widget if "name" port is connected
        auto updateWidget = [this, w, b_ = b.get()](PortId portId, bool connected){
            if (portId == m_inName)
            {
                w->setVisible(!connected);
                setNodeFlag(ResizableHOnly, !connected);
                b_->resize(connected ? b_->minimumSize() : size());

                emit nodeChanged();
            }
        };

        auto showWidget = [updateWidget](PortId portId){
            updateWidget(portId, false);
        };
        auto hideWidget = [updateWidget](PortId portId){
            updateWidget(portId, true);
        };

        updateWidget(m_inName, isPortConnected(m_inName));

        connect(this, &Node::portConnected, w, hideWidget);
        connect(this, &Node::portDisconnected, w, showWidget);

        // override functionality of select file path push button
        auto const& btns = w->findChildren<QPushButton*>();
        if (btns.empty()) return b;

        auto* btn = btns.last();
        if (!btn) return b;

        btn->disconnect();

        connect(btn, &QPushButton::clicked, this, [this, b_ = b.get()](){
            QString dir;
            if (auto const& dirData = nodeData<StringData>(m_inDir))
            {
                dir = dirData->value();
            }

            QString const& fileName =
                QFileDialog::getOpenFileName(b_, tr("Choose File"), dir);

            if (!fileName.isEmpty())
            {
                auto cmd = gtApp->makeCommand(this, tr("File Input changed"));
                Q_UNUSED(cmd);
                m_fileChooser.setVal(fileName);
            }
        });

        return b;
    });

    connect(&m_fileChooser, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);
}

void
FileInputNode::eval()
{
    QString const& filePath = m_fileChooser.get();

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
