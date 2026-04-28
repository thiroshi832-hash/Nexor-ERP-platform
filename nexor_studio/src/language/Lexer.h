// =============================================================================
// Lexer — converts Nexor source text into a Token stream.
//
//   - Whitespace is skipped (newlines preserved as Newline tokens).
//   - "'" begins a comment to end of line.
//   - "_" at end of line continues the next physical line.
//   - Identifiers + keywords are case-insensitive.
//   - Strings: "...""..." (doubled quote inside).
//   - Numbers: 123, 3.14, 1e6.
// =============================================================================
#ifndef NEXOR_STUDIO_LANG_LEXER_H
#define NEXOR_STUDIO_LANG_LEXER_H

#include "Token.h"
#include <QString>
#include <QVector>
#include <QHash>

namespace nx {

class Lexer {
public:
    explicit Lexer(QString source);

    QVector<Token> tokenize();          // throws on lex error
    QString lastError() const { return m_error; }
    bool    ok() const { return m_error.isEmpty(); }

private:
    bool   isAtEnd() const;
    QChar  peek(int offset = 0) const;
    QChar  advance();
    bool   match(QChar c);
    void   skipWhitespace();

    Token  readNumber();
    Token  readString();
    Token  readIdentOrKeyword();

    Token  make(TokKind k, const QString &lexeme = QString()) const;
    void   error(const QString &msg);

    QString          m_src;
    int              m_pos     { 0 };
    int              m_line    { 1 };
    int              m_col     { 1 };
    QString          m_error;
    static QHash<QString, TokKind> s_keywords;
    static void initKeywords();
};

} // namespace nx

#endif // NEXOR_STUDIO_LANG_LEXER_H
