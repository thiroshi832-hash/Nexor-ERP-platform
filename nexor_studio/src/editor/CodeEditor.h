// =============================================================================
// CodeEditor — QPlainTextEdit subclass with line-number gutter, current-line
// highlight, monospace font, and Nexor syntax highlighting.
//
// Loads / saves the <Code> CDATA section of an Atomic Activity (.aba file).
// =============================================================================
#ifndef NEXOR_STUDIO_CODEEDITOR_H
#define NEXOR_STUDIO_CODEEDITOR_H

#include <QPlainTextEdit>

class NexorHighlighter;
class LineNumberArea;

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    enum Kind { KindNone, KindActivity, KindForm };

    explicit CodeEditor(QWidget *parent = nullptr);
    ~CodeEditor() override;

    // Activity (.aba): Sub Main + globals.
    bool    loadActivity(const QString &abaPath);
    bool    saveActivity();

    // Form (.frm): event-driven code that lives alongside the widgets.
    bool    loadForm(const QString &frmPath);
    bool    saveForm();    // read-modify-write so widgets are preserved

    void    clearContent();

    Kind    currentKind()         const { return m_kind; }
    QString currentFilePath()     const { return m_path; }
    QString currentActivityPath() const   // backwards-compat
        { return (m_kind == KindActivity) ? m_path : QString(); }

    // Used by LineNumberArea
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int  lineNumberAreaWidth() const;

protected:
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    LineNumberArea   *m_lineNumberArea;
    NexorHighlighter *m_highlighter;
    QString           m_path;
    Kind              m_kind { KindNone };
};

// ────────────────────────────────────────────────────────────────────────────
// LineNumberArea — narrow widget on the left of the editor that paints
// gutter line numbers.
// ────────────────────────────────────────────────────────────────────────────
class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor *editor)
        : QWidget(editor), m_editor(editor) {}
    QSize sizeHint() const override
        { return QSize(m_editor->lineNumberAreaWidth(), 0); }

protected:
    void paintEvent(QPaintEvent *event) override
        { m_editor->lineNumberAreaPaintEvent(event); }

private:
    CodeEditor *m_editor;
};

#endif // NEXOR_STUDIO_CODEEDITOR_H
