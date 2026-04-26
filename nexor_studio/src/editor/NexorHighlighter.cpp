#include "NexorHighlighter.h"

NexorHighlighter::NexorHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {

    // Keywords (case-insensitive — VB Script convention)
    QTextCharFormat keywordFmt;
    keywordFmt.setForeground(QColor(0xc5, 0x86, 0xff));  // soft purple
    keywordFmt.setFontWeight(QFont::DemiBold);
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

    // Built-ins
    QTextCharFormat builtinFmt;
    builtinFmt.setForeground(QColor(0x4e, 0xc9, 0xb0));  // teal
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

    // Numbers
    QTextCharFormat numberFmt;
    numberFmt.setForeground(QColor(0xb5, 0xce, 0xa8));  // green-ish
    Rule numRule;
    numRule.pattern = QRegularExpression(R"(\b\d+(?:\.\d+)?\b)");
    numRule.format  = numberFmt;
    m_rules.append(numRule);

    // Strings — "..."  (no escapes in VB Script; "" inside is an escape)
    QTextCharFormat stringFmt;
    stringFmt.setForeground(QColor(0xce, 0x91, 0x78));  // warm orange
    Rule strRule;
    strRule.pattern = QRegularExpression(R"("(?:[^"]|"")*")");
    strRule.format  = stringFmt;
    m_rules.append(strRule);

    // Sub/Function names — capture identifier after Sub/Function
    QTextCharFormat funcDeclFmt;
    funcDeclFmt.setForeground(QColor(0xdc, 0xdc, 0xaa));  // pale yellow
    funcDeclFmt.setFontWeight(QFont::Bold);
    Rule fnRule;
    fnRule.pattern = QRegularExpression(R"(\b(?:Sub|Function)\s+([A-Za-z_]\w*))",
                                        QRegularExpression::CaseInsensitiveOption);
    fnRule.format  = funcDeclFmt;
    m_rules.append(fnRule);

    // Comments — apostrophe to end of line  (handled separately so it overrides
    // any keyword match inside the comment text)
    m_commentFormat.setForeground(QColor(0x6a, 0x99, 0x55));  // forest green
    m_commentFormat.setFontItalic(true);
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
