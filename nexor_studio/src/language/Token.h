// =============================================================================
// Token — lexical unit produced by Lexer, consumed by Parser.
//
// Nexor language tokens.  Names mirror VB-Script keywords; case-insensitive on
// the wire (the lexer normalises).
// =============================================================================
#ifndef NEXOR_STUDIO_LANG_TOKEN_H
#define NEXOR_STUDIO_LANG_TOKEN_H

#include <QString>

namespace nx {

enum class TokKind {
    // ── Literals
    Integer, Double, String,
    True, False, Nothing,
    // ── Identifier (and any keyword the parser hasn't elevated)
    Ident,
    // ── Keywords
    Sub, Function, End, Dim, Set, As,
    If, Then, Else, ElseIf,
    While, Wend, Do, Loop, Until,
    For, To, Step, Next,
    Return, Exit, Print,
    And, Or, Not, Mod,
    Call, Public, Private,
    From, Where, OrderBy, Select, Take, Ascending, Descending,
    // ── Operators / punctuation
    Plus, Minus, Star, Slash, Backslash, Caret, Ampersand,
    Eq, NotEq, Lt, LtEq, Gt, GtEq,
    LParen, RParen, LBracket, RBracket, Comma, Dot, Colon,
    Newline, Eof,
};

struct Token {
    TokKind kind   { TokKind::Eof };
    QString lexeme;       // raw text (preserves case)
    QString sval;         // for String literals: the unescaped contents
    double  nval { 0 };   // for Double literals
    qint64  ival { 0 };   // for Integer literals
    int     line { 1 };
    int     col  { 1 };

    static QString kindName(TokKind k);
};

} // namespace nx

#endif // NEXOR_STUDIO_LANG_TOKEN_H
