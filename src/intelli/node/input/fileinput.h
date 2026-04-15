/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_FILEINPUTNODE_H
#define GT_INTELLI_FILEINPUTNODE_H

#include <intelli/node.h>

#include <gt_openfilenameproperty.h>

namespace intelli
{

class FileInputNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE FileInputNode();

    /**
     * @brief Returns whether the external file-name input port is connected.
     * If connected, manual file picking UI should be hidden.
     */
    bool isFileNameInputConnected() const;

    /**
     * @brief Returns the currently selected file path stored in the node.
     */
    QString selectedFile() const;

    /**
     * @brief Returns the directory used as start location for file dialogs.
     * This is derived from the optional directory input port data.
     */
    QString dialogDirectory() const;

    /**
     * @brief Sets the selected file path and records the change in undo/redo.
     * @param filePath Absolute or relative path to the selected file.
     */
    void setSelectedFile(QString const& filePath);

signals:

    /**
     * @brief Emitted whenever the stored selected file path changes.
     */
    void selectedFileChanged(QString const& filePath);

    /**
     * @brief Emitted when the file-name input port connection state changes.
     */
    void fileNameInputConnectionChanged(bool connected);

protected:

    void eval() override;

private:

    PortId m_inDir, m_inName, m_outFile;
    GtOpenFileNameProperty m_fileChooser;
};

} // namespace intelli

#endif // GT_INTELLI_FILEINPUTNODE_H
