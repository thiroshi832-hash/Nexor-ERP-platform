#include "CodeEditor.h"
#include "NexorHighlighter.h"
#include "project/Activity.h"

#include <QPainter>
#include <QTextBlock>
#include <QFontMetrics>

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(nullptr)
    , m_highlighter(nullptr) {

    QFont mono("Consolas");
    mono.setStyleHint(QFont::Monospace);
    mono.setFixedPitch(true);
    mono.setPointSize(11);
    setFont(mono);

    setStyleSheet(R"(
        QPlainTextEdit {
            background:#1e1e1e; color:#dcdcdc;
            border:none; padding-left:2px;
            selection-background-color:#264f78;
        }
    )");

    setTabStopDistance(QFontMetricsF(font()).horizontalAdvance(' ') * 4);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setFrameShape(QFrame::NoFrame);

    m_lineNumberArea = new LineNumberArea(this);
    m_highlighter    = new NexorHighlighter(document());

    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

CodeEditor::~CodeEditor() = default;

// ─── Activity I/O ─────────────────────────────────────────────────────────
bool CodeEditor::loadActivity(const QString &abaPath) {
    Activity act;
    act.setFilePath(abaPath);
    if (!act.load()) {
        clearActivity();
        return false;
    }
    m_path = abaPath;
    setPlainText(act.code());
    return true;
}

bool CodeEditor::saveActivity() {
    if (m_path.isEmpty()) return false;
    Activity act;
    act.setFilePath(m_path);
    if (!act.load()) return false;     // load to preserve forms list + meta
    act.setCode(toPlainText());
    return act.save();
}

void CodeEditor::clearActivity() {
    m_path.clear();
    clear();
}

// ─── Line number gutter ───────────────────────────────────────────────────
int CodeEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return 12 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + 8;
}

void CodeEditor::updateLineNumberAreaWidth(int /*newBlockCount*/) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                        lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> sels;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(QColor(0x2a, 0x2d, 0x2e));
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = textCursor();
        sel.cursor.clearSelection();
        sels.append(sel);
    }
    setExtraSelections(sels);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(0x18, 0x18, 0x18));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top    = (int)blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int)blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(blockNumber == textCursor().blockNumber()
                            ? QColor(0xdc, 0xdc, 0xdc)
                            : QColor(0x6a, 0x6a, 0x6a));
            painter.drawText(0, top,
                             m_lineNumberArea->width() - 6, fontMetrics().height(),
                             Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + (int)blockBoundingRect(block).height();
        ++blockNumber;
    }
}
