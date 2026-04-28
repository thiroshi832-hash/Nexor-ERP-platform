#include "CodeEditor.h"
#include "NexorHighlighter.h"
#include "project/Activity.h"

#include <QPainter>
#include <QTextBlock>
#include <QFontMetrics>
#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(nullptr)
    , m_highlighter(nullptr) {

    // VB6 default: Courier New 10pt
    QFont mono("Courier New");
    mono.setStyleHint(QFont::Monospace);
    mono.setFixedPitch(true);
    mono.setPointSize(10);
    setFont(mono);

    setStyleSheet(R"(
        QPlainTextEdit {
            background:#ffffff; color:#000000;
            border:none; padding-left:2px;
            selection-background-color:#0a246a;
            selection-color:#ffffff;
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

// ─── Activity (.aba) I/O ──────────────────────────────────────────────────
bool CodeEditor::loadActivity(const QString &abaPath) {
    Activity act;
    act.setFilePath(abaPath);
    if (!act.load()) {
        clearContent();
        return false;
    }
    m_path = abaPath;
    m_kind = KindActivity;
    setPlainText(act.code());
    return true;
}

bool CodeEditor::saveActivity() {
    if (m_kind != KindActivity || m_path.isEmpty()) return false;
    Activity act;
    act.setFilePath(m_path);
    if (!act.load()) return false;     // load to preserve forms list + meta
    act.setCode(toPlainText());
    return act.save();
}

// ─── Form (.frm) I/O ──────────────────────────────────────────────────────
// We read the <Code> CDATA out of the .frm directly.  Saving does a
// read-modify-write so the <Widgets> block stays intact.
bool CodeEditor::loadForm(const QString &frmPath) {
    QFile f(frmPath);
    if (!f.open(QIODevice::ReadOnly)) {
        clearContent();
        return false;
    }
    QXmlStreamReader r(&f);
    QString code;
    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartElement() && r.name() == "Code") {
            code = r.readElementText();
            break;
        }
    }
    m_path = frmPath;
    m_kind = KindForm;
    setPlainText(code);
    return !r.hasError();
}

bool CodeEditor::saveForm() {
    if (m_kind != KindForm || m_path.isEmpty()) return false;

    // Read everything currently on disk so we can preserve the widgets block.
    QFile in(m_path);
    if (!in.open(QIODevice::ReadOnly)) return false;
    QByteArray raw = in.readAll();
    in.close();

    // Build a fresh document, copying every element verbatim except <Code>,
    // which we replace with the editor's current text.
    QXmlStreamReader r(raw);
    QByteArray out;
    QXmlStreamWriter w(&out);
    w.setAutoFormatting(true);
    bool wroteCode = false;
    int  formDepth = 0;

    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartDocument()) {
            w.writeStartDocument();
        } else if (r.isStartElement()) {
            ++formDepth;
            if (r.name() == "Code") {
                // Skip the original code content; emit our own.
                w.writeStartElement("Code");
                w.writeCDATA(toPlainText());
                w.writeEndElement();
                wroteCode = true;
                r.readElementText();  // consume original CDATA / text
                --formDepth;
            } else {
                w.writeStartElement(r.name().toString());
                w.writeAttributes(r.attributes());
            }
        } else if (r.isEndElement()) {
            // If we never saw <Code>, inject before closing the root <Form>.
            if (!wroteCode && formDepth == 1 && r.name() == "Form") {
                w.writeStartElement("Code");
                w.writeCDATA(toPlainText());
                w.writeEndElement();
                wroteCode = true;
            }
            w.writeEndElement();
            --formDepth;
        } else if (r.isCharacters() && !r.isWhitespace()) {
            w.writeCharacters(r.text().toString());
        } else if (r.isCDATA()) {
            w.writeCDATA(r.text().toString());
        } else if (r.isEndDocument()) {
            w.writeEndDocument();
        }
    }

    QFile out_f(m_path);
    if (!out_f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    out_f.write(out);
    return !r.hasError();
}

void CodeEditor::clearContent() {
    m_path.clear();
    m_kind = KindNone;
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

// Paint a thin separator above any block whose first non-whitespace token is
// "Sub" or "Function" — matches VB6's procedure-separator behaviour.
void CodeEditor::paintEvent(QPaintEvent *e) {
    QPlainTextEdit::paintEvent(e);

    QPainter painter(viewport());
    // VB6 procedure separator — solid black hairline.
    painter.setPen(QColor(0x00, 0x00, 0x00));

    QTextBlock block = firstVisibleBlock();
    while (block.isValid()) {
        QRectF g = blockBoundingGeometry(block).translated(contentOffset());
        if (g.top() > viewport()->height()) break;

        QString trimmed = block.text().trimmed();
        bool startsSub  = trimmed.startsWith(QStringLiteral("Sub "),      Qt::CaseInsensitive)
                       || trimmed.startsWith(QStringLiteral("Function "), Qt::CaseInsensitive);
        // Don't draw on the very first block (would clip at the top).
        if (startsSub && block.blockNumber() > 0)
            painter.drawLine(0, int(g.top()) - 1,
                             viewport()->width(), int(g.top()) - 1);
        block = block.next();
    }
}

void CodeEditor::highlightCurrentLine() {
    // VB6 has no current-line highlight; leave the selection list empty.
    setExtraSelections({});
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(m_lineNumberArea);
    // VB6 has a thin gray "margin indicator bar" on the left of the code
    // window for breakpoint dots; we widen it slightly to also show line
    // numbers.  Background is the system "button face" gray.
    painter.fillRect(event->rect(), QColor(0xee, 0xee, 0xee));
    // Right edge separator line
    painter.setPen(QColor(0xc0, 0xc0, 0xc0));
    painter.drawLine(m_lineNumberArea->width() - 1, event->rect().top(),
                     m_lineNumberArea->width() - 1, event->rect().bottom());

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top    = (int)blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int)blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(blockNumber == textCursor().blockNumber()
                            ? QColor(0x00, 0x00, 0x00)   // current = black
                            : QColor(0x80, 0x80, 0x80)); // others  = gray
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
