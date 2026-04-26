// =============================================================================
// NexorHighlighter — basic syntax highlighter for the Nexor language
// (VB-Script-flavoured: Sub/End Sub, Dim, If/Then/Else/End If, etc).
// =============================================================================
#ifndef NEXOR_STUDIO_NEXORHIGHLIGHTER_H
#define NEXOR_STUDIO_NEXORHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

class NexorHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit NexorHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat    format;
    };
    QVector<Rule>     m_rules;
    QTextCharFormat   m_commentFormat;
    QRegularExpression m_commentExpr;
};

#endif // NEXOR_STUDIO_NEXORHIGHLIGHTER_H
