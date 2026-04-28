#include "NexorHighlighter.h"

NexorHighlighter::NexorHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {

    // VB6 colours: keywords = pure blue, comments = dark green,
    // strings = no special colour (we use a subtle maroon for readability),
    // identifiers/numbers = default black.

    // Keywords (case-insensitive — VB Script convention)
    QTextCharFormat keywordFmt;
    keywordFmt.setForeground(QColor(0x00, 0x00, 0xff));  // pure blue, like VB6
    QStringList keywords = {
        "Sub", "End", "Function", "Dim", "Set", "If", "Then", "Else",
        "ElseIf", "While", "Wend", "Do", "Loop", "Until", "For", "Next",
        "To", "Step", "As", "Return", "Exit", "Goto", "Select", "Case",
        "And", "Or", "Not", "True", "False", "Nothing", "Print", "Stop"
    };
    for (const QString &kw : keywords) {
        Rule r;
        r.pattern = QRegularExpression(QString("\\b%1\\b").arg(kw),
                                       QRegularExpression::CaseInsensitiveOption);
        r.format = keywordFmt;
        m_rules.append(r);
    }

    // Built-ins — navy (slightly distinct from keywords)
    QTextCharFormat builtinFmt;
    builtinFmt.setForeground(QColor(0x00, 0x00, 0x80));  // navy
    QStringList builtins = {
        "GetProperty", "SetProperty", "CDbl", "CStr", "CInt", "CBool",
        "Len", "Mid", "Left", "Right", "UCase", "LCase", "Trim",
        "MsgBox", "InputBox", "Now", "Date", "Time"
    };
    for (const QString &b : builtins) {
        Rule r;
        r.pattern = QRegularExpression(QString("\\b%1\\b").arg(b),
                                       QRegularExpression::CaseInsensitiveOption);
        r.format = builtinFmt;
        m_rules.append(r);
    }

    // Numbers — VB6 leaves them at the identifier colour (black).  No rule.

    // Strings — "..."  (no escapes in VB Script; "" inside is an escape).
    // VB6 doesn't colour strings, but maroon is a near-universal convention
    // and helps quoted text pop on a white background.
    QTextCharFormat stringFmt;
    stringFmt.setForeground(QColor(0x80, 0x00, 0x00));  // maroon
    Rule strRule;
    strRule.pattern = QRegularExpression(R"("(?:[^"]|"")*")");
    strRule.format  = stringFmt;
    m_rules.append(strRule);

    // Comments — apostrophe to end of line.  VB6 colour: dark green, plain
    // (no italic).
    m_commentFormat.setForeground(QColor(0x00, 0x80, 0x00));  // dark green
    m_commentExpr = QRegularExpression(R"('[^\n]*)");
}

void NexorHighlighter::highlightBlock(const QString &text) {
    // First apply normal rules
    for (const Rule &r : m_rules) {
        auto it = r.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto m = it.next();
            int start  = m.capturedStart(m.lastCapturedIndex() > 0 ? 1 : 0);
            int length = m.capturedLength(m.lastCapturedIndex() > 0 ? 1 : 0);
            setFormat(start, length, r.format);
        }
    }
    // Comments override anything they overlap
    auto it = m_commentExpr.globalMatch(text);
    while (it.hasNext()) {
        auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), m_commentFormat);
    }
}
