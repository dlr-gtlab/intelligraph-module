#pragma once

#include <QWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QDialog>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QStackedLayout>

class MarkdownViewer : public QTextBrowser
{
    Q_OBJECT
public:
    explicit MarkdownViewer(QWidget* parent = nullptr);

signals:
    void editRequested();

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
};

class MarkdownEditor : public QTextEdit
{
    Q_OBJECT
public:
    explicit MarkdownEditor(QWidget* parent = nullptr);

signals:
    void editingFinished(const QString& text);

protected:
    void focusOutEvent(QFocusEvent* event) override;
};

class MarkdownWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MarkdownWindow(QString& text);

private:
    void setEditorMode();
    void setViewerMode();

    QString& m_text;

    MarkdownViewer* m_viewer;
    MarkdownEditor* m_editor;

    QStackedLayout* m_stack;
};
