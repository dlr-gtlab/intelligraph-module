#include "markdownwindow.h"

MarkdownViewer::MarkdownViewer(QWidget *parent)
    : QTextBrowser(parent)
{
    setReadOnly(true);
    setOpenLinks(false);
}

void MarkdownViewer::mouseDoubleClickEvent(QMouseEvent *event)
{
    emit editRequested();
    QTextBrowser::mouseDoubleClickEvent(event);
}

MarkdownEditor::MarkdownEditor(QWidget *parent)
    : QTextEdit(parent)
{}

void MarkdownEditor::focusOutEvent(QFocusEvent *event)
{
    emit editingFinished(toPlainText());
    QTextEdit::focusOutEvent(event);
}

MarkdownWindow::MarkdownWindow(QString& text)
    : QWidget(),
    m_text(text)
{
    m_viewer = new MarkdownViewer(this);
    m_editor = new MarkdownEditor(this);

    m_viewer->setMarkdown(m_text);
    m_editor->setText(m_text);

    m_stack = new QStackedLayout(this);

    m_stack->addWidget(m_viewer);
    m_stack->addWidget(m_editor);

    m_stack->setCurrentIndex(0);

    connect(m_viewer, &MarkdownViewer::editRequested, this, &MarkdownWindow::setEditorMode);

    connect(m_editor, &MarkdownEditor::editingFinished,
            this, [this](const QString& newText)
            {
                m_text = newText;
                m_viewer->setMarkdown(m_text);
                setViewerMode();
            });

    setViewerMode();
};

void
MarkdownWindow::setEditorMode()
{
    m_editor->setText(m_text);
    m_stack->setCurrentIndex(1);
    m_editor->setFocus();
}

void
MarkdownWindow::setViewerMode()
{
    m_viewer->setMarkdown(m_text);
    m_stack->setCurrentIndex(0);
}

