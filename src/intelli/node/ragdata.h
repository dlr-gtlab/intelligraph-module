/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Robert Marcenko <robert.marcenko@dlr.de>
 */
#pragma once


#include <intelli/DynamicNode.h>
#include <intelli/gui/widgets/markdownwindow.h>

#include <QDialog>
#include <QToolButton>
#include <QWidget>
#include <QStackedLayout>

#include <gt_enumproperty.h>
#include <gt_boolproperty.h>
#include <gt_stringproperty.h>
#include <gt_icons.h>
#include <gt_openfilenameproperty.h>


class QHBoxLayout;
class QVBoxLayout;
class QLabel;

namespace intelli
{

class RagDataNode : public DynamicNode
{
    Q_OBJECT

public:

    Q_INVOKABLE RagDataNode();

    GtOpenFileNameProperty m_pathToImage{"Path","Path","Path",{""}};
    QString text(int i) const;
    void setText(QString text,int i);

signals:
    void TextChanged(const QString&);

private:
    GtBoolProperty m_hideButton{"buttonvisiblity", tr("Hide Button"),tr("Hide the button on the node"),false};

    GtStringProperty m_text{"Text","Text"};
    GtStringProperty m_text1{"Text1","Text 1"};
    GtStringProperty m_text2{"Text2","Text 2"};

};


class RAGDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RAGDialog(RagDataNode& node, QWidget* parent = nullptr);

    void saveText();

private:
    RagDataNode& m_node;
    QStringList m_text;
    MarkdownWindow* m_window;
    MarkdownWindow* m_window1;
};

class RagWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RagWidget(RagDataNode& node, QWidget* parent = nullptr);;

    void showContext();;

public slots:

private:
    RagDataNode& m_node;
    QToolButton* Button;

};

} // namespace intelli

