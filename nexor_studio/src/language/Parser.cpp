#include "Parser.h"

namespace nx {

Parser::Parser(QVector<Token> tokens) : m_tokens(std::move(tokens)) {}

// ── Token cursor ─────────────────────────────────────────────────────────
const Token& Parser::peek(int off) const {
    int p = m_pos + off;
    if (p < 0 || p >= m_tokens.size()) return m_tokens.last();
    return m_tokens[p];
}
const Token& Parser::advance() {
    const Token &t = m_tokens[m_pos];
    if (m_pos < m_tokens.size() - 1) ++m_pos;
    return t;
}
bool Parser::check(TokKind k) const { return peek().kind == k; }
bool Parser::match(TokKind k)       { if (check(k)) { advance(); return true; } return false; }
bool Parser::matchAny(std::initializer_list<TokKind> ks) {
    for (auto k : ks) if (check(k)) { advance(); return true; }
    return false;
}
bool Parser::consumeNewline() {
    if (check(TokKind::Newline) || check(TokKind::Eof)) { advance(); return true; }
    return false;
}
void Parser::skipNewlines() {
    while (check(TokKind::Newline)) advance();
}

void Parser::errorAt(const Token &t, const QString &msg) {
    if (m_error.isEmpty()) {
        m_error = QString("[line %1] %2").arg(t.line).arg(msg);
        m_errorLine = t.line;
    }
}
Token Parser::expect(TokKind k, const QString &what) {
    if (check(k)) return advance();
    errorAt(peek(), QString("expected %1 but got '%2'").arg(what, peek().lexeme.isEmpty()
                                                                  ? Token::kindName(peek().kind)
                                                                  : peek().lexeme));
    return Token{};
}

// ── Top level ────────────────────────────────────────────────────────────
Program Parser::parse() {
    Program p;
    skipNewlines();
    while (!check(TokKind::Eof) && m_error.isEmpty()) {
        // Strip leading Public/Private
        bool _ = matchAny({TokKind::Public, TokKind::Private});
        Q_UNUSED(_);
        if (match(TokKind::Sub))      { auto s = parseSub(false); if (s) p.subs.append(s); }
        else if (match(TokKind::Function)) { auto s = parseSub(true);  if (s) p.subs.append(s); }
        else {
            auto s = parseStmt();
            if (s) p.topLevel.append(s);
        }
        skipNewlines();
    }
    return p;
}

// ── Sub / Function ────────────────────────────────────────────────────────
SubPtr Parser::parseSub(bool isFunction) {
    auto s = std::make_shared<SubDecl>();
    s->isFunction = isFunction;
    Token name = expect(TokKind::Ident,
                        isFunction ? "function name" : "sub name");
    s->name = name.lexeme;
    s->line = name.line;
    if (match(TokKind::LParen)) {
        if (!check(TokKind::RParen)) {
            do {
                Token p = expect(TokKind::Ident, "parameter name");
                s->params << p.lexeme;
                // Skip optional "As Type"
                if (match(TokKind::As)) {
                    if (check(TokKind::Ident)) advance();
                }
            } while (match(TokKind::Comma));
        }
        expect(TokKind::RParen, "')'");
    }
    // Function may have a return-type annotation: As Ident
    if (isFunction && match(TokKind::As)) {
        if (check(TokKind::Ident)) advance();
    }
    consumeNewline();
    s->body = parseStmts({TokKind::End});
    expect(TokKind::End, "'End'");
    if (isFunction) expect(TokKind::Function, "'Function'");
    else            expect(TokKind::Sub,      "'Sub'");
    consumeNewline();
    return m_error.isEmpty() ? s : nullptr;
}

QVector<StmtPtr> Parser::parseStmts(std::initializer_list<TokKind> stopKinds) {
    QVector<StmtPtr> out;
    skipNewlines();
    while (!check(TokKind::Eof) && m_error.isEmpty()) {
        for (auto k : stopKinds) if (check(k)) return out;
        // ElseIf and Else also stop nested blocks (handled by caller)
        if (check(TokKind::ElseIf) || check(TokKind::Else)) return out;
        if (check(TokKind::Wend) || check(TokKind::Next) || check(TokKind::Loop))
            return out;

        auto s = parseStmt();
        if (s) out.append(s);
        skipNewlines();
    }
    return out;
}

// ── Statements ───────────────────────────────────────────────────────────
StmtPtr Parser::parseStmt() {
    if (check(TokKind::Dim))    return parseDim();
    if (check(TokKind::If))     return parseIf();
    if (check(TokKind::While))  return parseWhile();
    if (check(TokKind::For))    return parseFor();
    if (check(TokKind::Print))  return parsePrint();
    if (check(TokKind::Return)) return parseReturn();
    if (check(TokKind::Exit))   return parseExit();
    if (match(TokKind::Set))    {
        // Set name = expr  — same as assign for our value-type world.
        return parseAssignOrExpr();
    }
    if (match(TokKind::Call))   {
        // Call expr — optional Call keyword before a sub call.
        auto e = parseExpr();
        consumeNewline();
        return std::make_shared<ExprStatement>(e ? e->line : peek().line, e);
    }
    return parseAssignOrExpr();
}

StmtPtr Parser::parseDim() {
    int line = peek().line;
    advance();                          // consume Dim
    Token name = expect(TokKind::Ident, "variable name");
    if (match(TokKind::As)) { if (check(TokKind::Ident)) advance(); }
    ExprPtr init;
    if (match(TokKind::Eq)) init = parseExpr();
    consumeNewline();
    return std::make_shared<DimStatement>(line, name.lexeme, init);
}

StmtPtr Parser::parseAssignOrExpr() {
    // Look ahead: Ident "=" → assignment; otherwise expression statement.
    if (check(TokKind::Ident) && peek(1).kind == TokKind::Eq) {
        Token name = advance();
        advance();                      // consume =
        ExprPtr v = parseExpr();
        consumeNewline();
        return std::make_shared<AssignStatement>(name.line, name.lexeme, v);
    }
    int line = peek().line;
    ExprPtr e = parseExpr();
    consumeNewline();
    return std::make_shared<ExprStatement>(line, e);
}

StmtPtr Parser::parseIf() {
    int line = peek().line;
    advance();                          // consume If
    auto s = std::make_shared<IfStatement>(line);
    ExprPtr cond = parseExpr();
    expect(TokKind::Then, "'Then'");
    consumeNewline();
    QVector<StmtPtr> body = parseStmts({TokKind::End, TokKind::Else, TokKind::ElseIf});
    s->branches.append({cond, body});

    while (match(TokKind::ElseIf)) {
        ExprPtr c2 = parseExpr();
        expect(TokKind::Then, "'Then'");
        consumeNewline();
        QVector<StmtPtr> b2 = parseStmts({TokKind::End, TokKind::Else, TokKind::ElseIf});
        s->branches.append({c2, b2});
    }
    if (match(TokKind::Else)) {
        consumeNewline();
        s->elseBody = parseStmts({TokKind::End});
    }
    expect(TokKind::End, "'End'");
    expect(TokKind::If,  "'If'");
    consumeNewline();
    return s;
}

StmtPtr Parser::parseWhile() {
    int line = peek().line;
    advance();                          // consume While
    ExprPtr cond = parseExpr();
    consumeNewline();
    QVector<StmtPtr> body = parseStmts({TokKind::Wend});
    expect(TokKind::Wend, "'Wend'");
    consumeNewline();
    return std::make_shared<WhileStatement>(line, cond, body);
}

StmtPtr Parser::parseFor() {
    int line = peek().line;
    advance();                          // consume For
    auto s = std::make_shared<ForStatement>(line);
    Token name = expect(TokKind::Ident, "loop variable");
    s->var = name.lexeme;
    expect(TokKind::Eq, "'='");
    s->start = parseExpr();
    expect(TokKind::To, "'To'");
    s->end = parseExpr();
    if (match(TokKind::Step)) s->step = parseExpr();
    consumeNewline();
    s->body = parseStmts({TokKind::Next});
    expect(TokKind::Next, "'Next'");
    if (check(TokKind::Ident)) advance();    // optional loop-var name after Next
    consumeNewline();
    return s;
}

StmtPtr Parser::parsePrint() {
    int line = peek().line;
    advance();                          // consume Print
    ExprPtr e;
    if (!check(TokKind::Newline) && !check(TokKind::Eof)) e = parseExpr();
    consumeNewline();
    return std::make_shared<PrintStatement>(line, e);
}

StmtPtr Parser::parseReturn() {
    int line = peek().line;
    advance();                          // consume Return
    ExprPtr v;
    if (!check(TokKind::Newline) && !check(TokKind::Eof)) v = parseExpr();
    consumeNewline();
    return std::make_shared<ReturnStatement>(line, v);
}

StmtPtr Parser::parseExit() {
    int line = peek().line;
    advance();                          // consume Exit
    ExitStatement::What w = ExitStatement::ExitSub;
    if      (match(TokKind::Sub))      w = ExitStatement::ExitSub;
    else if (match(TokKind::Function)) w = ExitStatement::ExitFunction;
    else if (match(TokKind::For))      w = ExitStatement::ExitFor;
    else if (match(TokKind::While))    w = ExitStatement::ExitWhile;
    else if (match(TokKind::Do))       w = ExitStatement::ExitDo;
    consumeNewline();
    return std::make_shared<ExitStatement>(line, w);
}

// ── Expressions ──────────────────────────────────────────────────────────
ExprPtr Parser::parseExpr()   { return parseOr(); }

ExprPtr Parser::parseOr() {
    ExprPtr l = parseAnd();
    while (check(TokKind::Or)) {
        int ln = peek().line; advance();
        ExprPtr r = parseAnd();
        l = std::make_shared<LogicalExpr>(ln, TokKind::Or, l, r);
    }
    return l;
}
ExprPtr Parser::parseAnd() {
    ExprPtr l = parseNot();
    while (check(TokKind::And)) {
        int ln = peek().line; advance();
        ExprPtr r = parseNot();
        l = std::make_shared<LogicalExpr>(ln, TokKind::And, l, r);
    }
    return l;
}
ExprPtr Parser::parseNot() {
    if (check(TokKind::Not)) {
        int ln = peek().line; advance();
        ExprPtr e = parseNot();
        return std::make_shared<UnaryExpr>(ln, TokKind::Not, e);
    }
    return parseCmp();
}
ExprPtr Parser::parseCmp() {
    ExprPtr l = parseConcat();
    while (true) {
        TokKind k = peek().kind;
        if (k == TokKind::Eq || k == TokKind::NotEq ||
            k == TokKind::Lt || k == TokKind::LtEq ||
            k == TokKind::Gt || k == TokKind::GtEq) {
            int ln = peek().line; advance();
            ExprPtr r = parseConcat();
            l = std::make_shared<BinaryExpr>(ln, k, l, r);
        } else break;
    }
    return l;
}
ExprPtr Parser::parseConcat() {
    ExprPtr l = parseAdd();
    while (check(TokKind::Ampersand)) {
        int ln = peek().line; advance();
        ExprPtr r = parseAdd();
        l = std::make_shared<BinaryExpr>(ln, TokKind::Ampersand, l, r);
    }
    return l;
}
ExprPtr Parser::parseAdd() {
    ExprPtr l = parseMul();
    while (check(TokKind::Plus) || check(TokKind::Minus)) {
        int ln = peek().line; TokKind op = advance().kind;
        ExprPtr r = parseMul();
        l = std::make_shared<BinaryExpr>(ln, op, l, r);
    }
    return l;
}
ExprPtr Parser::parseMul() {
    ExprPtr l = parseUnary();
    while (check(TokKind::Star) || check(TokKind::Slash) || check(TokKind::Mod)) {
        int ln = peek().line; TokKind op = advance().kind;
        ExprPtr r = parseUnary();
        l = std::make_shared<BinaryExpr>(ln, op, l, r);
    }
    return l;
}
ExprPtr Parser::parseUnary() {
    if (check(TokKind::Minus)) {
        int ln = peek().line; advance();
        ExprPtr e = parseUnary();
        return std::make_shared<UnaryExpr>(ln, TokKind::Minus, e);
    }
    return parsePostfix();
}
ExprPtr Parser::parsePostfix() {
    ExprPtr e = parsePrimary();
    while (true) {
        if (check(TokKind::Dot)) {
            advance();
            Token p = expect(TokKind::Ident, "property name");
            e = std::make_shared<MemberExpr>(p.line, e, p.lexeme);
            continue;
        }
        // Call: an Ident followed by ( becomes a CallExpr (handled in
        // parsePrimary as well; here we don't allow chained calls).
        break;
    }
    return e;
}
ExprPtr Parser::parsePrimary() {
    const Token &t = peek();
    switch (t.kind) {
    case TokKind::Integer: advance(); return std::make_shared<LiteralExpr>(t.line, Value::integer(t.ival));
    case TokKind::Double:  advance(); return std::make_shared<LiteralExpr>(t.line, Value::real(t.nval));
    case TokKind::String:  advance(); return std::make_shared<LiteralExpr>(t.line, Value::text(t.sval));
    case TokKind::True:    advance(); return std::make_shared<LiteralExpr>(t.line, Value::boolean(true));
    case TokKind::False:   advance(); return std::make_shared<LiteralExpr>(t.line, Value::boolean(false));
    case TokKind::Nothing: advance(); return std::make_shared<LiteralExpr>(t.line, Value::nothing());
    case TokKind::LParen: {
        advance();
        ExprPtr e = parseExpr();
        expect(TokKind::RParen, "')'");
        return e;
    }
    case TokKind::Ident: {
        advance();
        if (match(TokKind::LParen)) {
            QVector<ExprPtr> args = parseArgs();
            expect(TokKind::RParen, "')'");
            return std::make_shared<CallExpr>(t.line, t.lexeme, args);
        }
        return std::make_shared<VariableExpr>(t.line, t.lexeme);
    }
    default:
        errorAt(t, QString("expected expression but got '%1'").arg(
                   t.lexeme.isEmpty() ? Token::kindName(t.kind) : t.lexeme));
        return std::make_shared<LiteralExpr>(t.line, Value());
    }
}
QVector<ExprPtr> Parser::parseArgs() {
    QVector<ExprPtr> args;
    if (check(TokKind::RParen)) return args;
    args.append(parseExpr());
    while (match(TokKind::Comma)) args.append(parseExpr());
    return args;
}

} // namespace nx
