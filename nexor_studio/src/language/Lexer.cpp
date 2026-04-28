#include "Lexer.h"
#include <QChar>

namespace nx {

QHash<QString, TokKind> Lexer::s_keywords;

void Lexer::initKeywords() {
    if (!s_keywords.isEmpty()) return;
    s_keywords = {
        { "sub",      TokKind::Sub      },
        { "function", TokKind::Function },
        { "end",      TokKind::End      },
        { "dim",      TokKind::Dim      },
        { "set",      TokKind::Set      },
        { "as",       TokKind::As       },
        { "if",       TokKind::If       },
        { "then",     TokKind::Then     },
        { "else",     TokKind::Else     },
        { "elseif",   TokKind::ElseIf   },
        { "while",    TokKind::While    },
        { "wend",     TokKind::Wend     },
        { "do",       TokKind::Do       },
        { "loop",     TokKind::Loop     },
        { "until",    TokKind::Until    },
        { "for",      TokKind::For      },
        { "to",       TokKind::To       },
        { "step",     TokKind::Step     },
        { "next",     TokKind::Next     },
        { "return",   TokKind::Return   },
        { "exit",     TokKind::Exit     },
        { "print",    TokKind::Print    },
        { "and",      TokKind::And      },
        { "or",       TokKind::Or       },
        { "not",      TokKind::Not      },
        { "mod",      TokKind::Mod      },
        { "call",     TokKind::Call     },
        { "public",   TokKind::Public   },
        { "private",  TokKind::Private  },
        { "true",     TokKind::True     },
        { "false",    TokKind::False    },
        { "nothing",  TokKind::Nothing  },
        { "from",       TokKind::From       },
        { "where",      TokKind::Where      },
        { "orderby",    TokKind::OrderBy    },
        { "select",     TokKind::Select     },
        { "take",       TokKind::Take       },
        { "ascending",  TokKind::Ascending  },
        { "descending", TokKind::Descending },
    };
}

Lexer::Lexer(QString source) : m_src(std::move(source)) { initKeywords(); }

bool Lexer::isAtEnd() const { return m_pos >= m_src.size(); }

QChar Lexer::peek(int off) const {
    int p = m_pos + off;
    return (p < m_src.size()) ? m_src[p] : QChar('\0');
}

QChar Lexer::advance() {
    QChar c = m_src[m_pos++];
    if (c == '\n') { ++m_line; m_col = 1; } else { ++m_col; }
    return c;
}

bool Lexer::match(QChar c) {
    if (isAtEnd() || m_src[m_pos] != c) return false;
    advance();
    return true;
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        QChar c = peek();
        if (c == ' ' || c == '\t' || c == '\r') { advance(); }
        else if (c == '\'')  {  // line comment
            while (!isAtEnd() && peek() != '\n') advance();
        }
        else if (c == '_' && peek(1) == '\n') {
            // line-continuation: consume the underscore + newline
            advance(); advance();
        }
        else break;
    }
}

Token Lexer::make(TokKind k, const QString &lex) const {
    Token t; t.kind = k; t.lexeme = lex; t.line = m_line; t.col = m_col;
    return t;
}

void Lexer::error(const QString &msg) {
    if (m_error.isEmpty())
        m_error = QString("[line %1, col %2] %3").arg(m_line).arg(m_col).arg(msg);
}

Token Lexer::readNumber() {
    int start = m_pos;
    int line = m_line, col = m_col;
    while (peek().isDigit()) advance();
    bool isFloat = false;
    if (peek() == '.' && peek(1).isDigit()) {
        isFloat = true;
        advance();                          // consume '.'
        while (peek().isDigit()) advance();
    }
    if (peek().toLower() == 'e') {
        isFloat = true;
        advance();
        if (peek() == '+' || peek() == '-') advance();
        while (peek().isDigit()) advance();
    }
    QString lex = m_src.mid(start, m_pos - start);
    Token t;
    t.lexeme = lex; t.line = line; t.col = col;
    if (isFloat) {
        t.kind = TokKind::Double;
        t.nval = lex.toDouble();
    } else {
        t.kind = TokKind::Integer;
        t.ival = lex.toLongLong();
    }
    return t;
}

Token Lexer::readString() {
    int line = m_line, col = m_col;
    advance();                              // consume opening "
    QString out;
    while (!isAtEnd()) {
        QChar c = peek();
        if (c == '"') {
            if (peek(1) == '"') {           // "" → literal "
                advance(); advance();
                out += '"';
            } else {
                advance();                  // closing quote
                Token t;
                t.kind   = TokKind::String;
                t.sval   = out;
                t.lexeme = "\"" + out + "\"";
                t.line   = line; t.col = col;
                return t;
            }
        } else if (c == '\n') {
            error("unterminated string literal"); break;
        } else {
            out += c; advance();
        }
    }
    error("unterminated string literal");
    return make(TokKind::Eof);
}

Token Lexer::readIdentOrKeyword() {
    int start = m_pos, line = m_line, col = m_col;
    while (peek().isLetterOrNumber() || peek() == '_') advance();
    QString lex = m_src.mid(start, m_pos - start);
    QString lo  = lex.toLower();
    auto it = s_keywords.find(lo);
    Token t;
    t.kind   = (it != s_keywords.end()) ? it.value() : TokKind::Ident;
    t.lexeme = lex;
    t.line = line; t.col = col;
    if      (t.kind == TokKind::True)    { t.kind = TokKind::True; }
    else if (t.kind == TokKind::False)   { t.kind = TokKind::False; }
    else if (t.kind == TokKind::Nothing) { t.kind = TokKind::Nothing; }
    return t;
}

QVector<Token> Lexer::tokenize() {
    QVector<Token> out;
    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;
        QChar c = peek();
        int line = m_line, col = m_col;

        // Newlines are statement terminators.
        if (c == '\n') {
            advance();
            // Coalesce consecutive newlines into one
            if (!out.isEmpty() && out.last().kind != TokKind::Newline)
                out.append(make(TokKind::Newline, "\n"));
            continue;
        }
        // ':' is normally the in-line statement separator (treat as Newline
        // for parser).  ':=' is the named-argument operator used inside
        // [Activity(Key := Value)] annotations.
        if (c == ':') {
            advance();
            if (peek() == QChar('=')) {
                advance();
                Token t; t.line = line; t.col = col;
                t.kind = TokKind::Colon; t.lexeme = ":=";
                out.append(t);                      // Colon ; Eq follows
                Token e; e.line = line; e.col = col;
                e.kind = TokKind::Eq; e.lexeme = "=";
                out.append(e);
                continue;
            }
            if (!out.isEmpty() && out.last().kind != TokKind::Newline)
                out.append(make(TokKind::Newline, ":"));
            continue;
        }
        if (c.isDigit())              { out.append(readNumber()); continue; }
        if (c == '"')                 { out.append(readString()); continue; }
        if (c.isLetter() || c == '_') { out.append(readIdentOrKeyword()); continue; }

        // Single / double character punctuation
        Token tok; tok.line = line; tok.col = col;
        switch (c.unicode()) {
        case '+': advance(); tok.kind = TokKind::Plus;    tok.lexeme = "+"; break;
        case '-': advance(); tok.kind = TokKind::Minus;   tok.lexeme = "-"; break;
        case '*': advance(); tok.kind = TokKind::Star;    tok.lexeme = "*"; break;
        case '/': advance(); tok.kind = TokKind::Slash;   tok.lexeme = "/"; break;
        case '\\':advance(); tok.kind = TokKind::Backslash;tok.lexeme= "\\"; break;
        case '^': advance(); tok.kind = TokKind::Caret;   tok.lexeme = "^"; break;
        case '&': advance(); tok.kind = TokKind::Ampersand;tok.lexeme= "&"; break;
        case '(': advance(); tok.kind = TokKind::LParen;  tok.lexeme = "("; break;
        case ')': advance(); tok.kind = TokKind::RParen;  tok.lexeme = ")"; break;
        case '[': advance(); tok.kind = TokKind::LBracket;tok.lexeme = "["; break;
        case ']': advance(); tok.kind = TokKind::RBracket;tok.lexeme = "]"; break;
        case ',': advance(); tok.kind = TokKind::Comma;   tok.lexeme = ","; break;
        case '.': advance(); tok.kind = TokKind::Dot;     tok.lexeme = "."; break;
        case '=': advance(); tok.kind = TokKind::Eq;      tok.lexeme = "="; break;
        case '<':
            advance();
            if (match('='))      { tok.kind = TokKind::LtEq;   tok.lexeme = "<="; }
            else if (match('>')) { tok.kind = TokKind::NotEq;  tok.lexeme = "<>"; }
            else                 { tok.kind = TokKind::Lt;     tok.lexeme = "<";  }
            break;
        case '>':
            advance();
            if (match('=')) { tok.kind = TokKind::GtEq; tok.lexeme = ">="; }
            else            { tok.kind = TokKind::Gt;   tok.lexeme = ">";  }
            break;
        default:
            advance();
            error(QString("unexpected character '%1'").arg(c));
            continue;
        }
        out.append(tok);
    }
    Token eof; eof.kind = TokKind::Eof; eof.line = m_line; eof.col = m_col;
    out.append(eof);
    return out;
}

QString Token::kindName(TokKind k) {
    switch (k) {
    case TokKind::Integer:   return "integer";
    case TokKind::Double:    return "double";
    case TokKind::String:    return "string";
    case TokKind::True:      return "True";
    case TokKind::False:     return "False";
    case TokKind::Nothing:   return "Nothing";
    case TokKind::Ident:     return "identifier";
    case TokKind::Sub:       return "Sub";
    case TokKind::Function:  return "Function";
    case TokKind::End:       return "End";
    case TokKind::Dim:       return "Dim";
    case TokKind::Set:       return "Set";
    case TokKind::As:        return "As";
    case TokKind::If:        return "If";
    case TokKind::Then:      return "Then";
    case TokKind::Else:      return "Else";
    case TokKind::ElseIf:    return "ElseIf";
    case TokKind::While:     return "While";
    case TokKind::Wend:      return "Wend";
    case TokKind::Do:        return "Do";
    case TokKind::Loop:      return "Loop";
    case TokKind::Until:     return "Until";
    case TokKind::For:       return "For";
    case TokKind::To:        return "To";
    case TokKind::Step:      return "Step";
    case TokKind::Next:      return "Next";
    case TokKind::Return:    return "Return";
    case TokKind::Exit:      return "Exit";
    case TokKind::Print:     return "Print";
    case TokKind::And:       return "And";
    case TokKind::Or:        return "Or";
    case TokKind::Not:       return "Not";
    case TokKind::Mod:       return "Mod";
    case TokKind::Call:      return "Call";
    case TokKind::Public:    return "Public";
    case TokKind::Private:   return "Private";
    case TokKind::From:        return "From";
    case TokKind::Where:       return "Where";
    case TokKind::OrderBy:     return "OrderBy";
    case TokKind::Select:      return "Select";
    case TokKind::Take:        return "Take";
    case TokKind::Ascending:   return "Ascending";
    case TokKind::Descending:  return "Descending";
    case TokKind::Plus:      return "+";
    case TokKind::Minus:     return "-";
    case TokKind::Star:      return "*";
    case TokKind::Slash:     return "/";
    case TokKind::Backslash: return "\\";
    case TokKind::Caret:     return "^";
    case TokKind::Ampersand: return "&";
    case TokKind::Eq:        return "=";
    case TokKind::NotEq:     return "<>";
    case TokKind::Lt:        return "<";
    case TokKind::LtEq:      return "<=";
    case TokKind::Gt:        return ">";
    case TokKind::GtEq:      return ">=";
    case TokKind::LParen:    return "(";
    case TokKind::RParen:    return ")";
    case TokKind::LBracket:  return "[";
    case TokKind::RBracket:  return "]";
    case TokKind::Comma:     return ",";
    case TokKind::Dot:       return ".";
    case TokKind::Colon:     return ":";
    case TokKind::Newline:   return "newline";
    case TokKind::Eof:       return "end-of-file";
    }
    return "?";
}

} // namespace nx
