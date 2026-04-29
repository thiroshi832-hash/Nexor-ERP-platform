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
        // Optional [Activity(...)] annotation immediately preceding a Sub
        // or Function.  Accept multiple annotation blocks; later ones
        // override earlier keys with the same name.
        SubAnnotation pending;
        while (check(TokKind::LBracket)) {
            SubAnnotation a = parseAnnotation();
            if (!m_error.isEmpty()) return p;
            for (auto it = a.args.constBegin(); it != a.args.constEnd(); ++it)
                pending.args.insert(it.key(), it.value());
            skipNewlines();
        }
        // Strip leading Public/Private
        bool _ = matchAny({TokKind::Public, TokKind::Private});
        Q_UNUSED(_);
        if (match(TokKind::Sub))      {
            auto s = parseSub(false);
            if (s) { s->annotation = pending; p.subs.append(s); }
        }
        else if (match(TokKind::Function)) {
            auto s = parseSub(true);
            if (s) { s->annotation = pending; p.subs.append(s); }
        }
        else {
            // Annotations on bare statements aren't allowed - but we
            // tolerate the syntax silently in case the user is mid-edit.
            auto s = parseStmt();
            if (s) p.topLevel.append(s);
        }
        skipNewlines();
    }
    return p;
}

// ── [Activity(Key := Value, ...)] ────────────────────────────────────────
//
// We accept the "name(args)" form even though only "Activity" is
// meaningful today, because the architecture (page 6) shows the same
// shape for future annotations like [Reports(...)] or [Schedule(...)].
SubAnnotation Parser::parseAnnotation() {
    SubAnnotation out;
    expect(TokKind::LBracket, "'['");
    if (check(TokKind::Ident)) advance();      // annotation kind, e.g. "Activity"
    if (match(TokKind::LParen)) {
        if (!check(TokKind::RParen)) {
            do {
                Token key = expect(TokKind::Ident, "annotation key");
                if (m_error.isEmpty()) {
                    // Accept ':=' (Colon then Eq), or a bare '=', as the
                    // assignment operator inside annotations.
                    bool ok = (match(TokKind::Colon) && match(TokKind::Eq))
                              || match(TokKind::Eq);
                    if (!ok) {
                        errorAt(peek(), "expected ':=' after annotation key");
                        return out;
                    }
                    Value v;
                    const Token &t = peek();
                    if (t.kind == TokKind::String) {
                        v = Value::text(t.sval);
                        advance();
                    } else if (t.kind == TokKind::Integer) {
                        v = Value::integer(t.ival);
                        advance();
                    } else if (t.kind == TokKind::Double) {
                        v = Value::real(t.nval);
                        advance();
                    } else if (t.kind == TokKind::True || t.kind == TokKind::False) {
                        v = Value::boolean(t.kind == TokKind::True);
                        advance();
                    } else if (t.kind == TokKind::Ident) {
                        // Identifier: treat as a string token so the host
                        // can compare against "ServerOnly" / "ClientOnly".
                        v = Value::text(t.lexeme);
                        advance();
                    } else {
                        errorAt(t, "expected literal in annotation argument");
                        return out;
                    }
                    out.args.insert(key.lexeme.toLower(), v);
                }
            } while (match(TokKind::Comma));
        }
        expect(TokKind::RParen, "')'");
    }
    expect(TokKind::RBracket, "']'");
    return out;
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
    // Parse a primary-then-postfix expression; if it ends with a member
    // access AND is followed by '=', it's a member-assignment statement.
    ExprPtr e = parseExpr();
    if (check(TokKind::Eq) && e && e->kind == Expr::Member) {
        advance();                      // consume =
        ExprPtr v = parseExpr();
        consumeNewline();
        auto *m = static_cast<MemberExpr*>(e.get());
        return std::make_shared<MemberAssignStatement>(line,
                                                      m->object,
                                                      m->property,
                                                      v);
    }
    consumeNewline();
    return std::make_shared<ExprStatement>(line, e);
}

StmtPtr Parser::parseIf() {
    int line = peek().line;
    advance();                          // consume If
    auto s = std::make_shared<IfStatement>(line);
    ExprPtr cond = parseExpr();
    expect(TokKind::Then, "'Then'");

    // Single-line form: If <cond> Then <stmt> [Else <stmt>]
    // (one statement after Then, on the same line; no End If).  The
    // marker is "did NOT see a newline immediately after Then".
    if (!check(TokKind::Newline) && !check(TokKind::Eof)) {
        QVector<StmtPtr> body;
        StmtPtr first = parseStmt();
        if (first) body.append(first);
        s->branches.append({cond, body});
        if (match(TokKind::Else)) {
            QVector<StmtPtr> elseBody;
            StmtPtr e = parseStmt();
            if (e) elseBody.append(e);
            s->elseBody = elseBody;
        }
        consumeNewline();
        return s;
    }

    // Multi-line form: If <cond> Then <NL> stmts {ElseIf...} [Else stmts]
    //                  End If
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

    // For Each <ident> In <collection> ... Next
    if (check(TokKind::Ident) && peek().lexeme.compare("Each", Qt::CaseInsensitive) == 0) {
        advance();                      // consume Each
        auto fe = std::make_shared<ForEachStatement>(line);
        Token name = expect(TokKind::Ident, "loop variable");
        fe->var = name.lexeme;
        // 'In' isn't a reserved keyword in our lexer — it'll be an Ident.
        if (check(TokKind::Ident) && peek().lexeme.compare("In", Qt::CaseInsensitive) == 0) {
            advance();
        } else {
            errorAt(peek(), "expected 'In' after For Each variable");
        }
        fe->collection = parseExpr();
        consumeNewline();
        fe->body = parseStmts({TokKind::Next});
        expect(TokKind::Next, "'Next'");
        if (check(TokKind::Ident)) advance();   // optional loop-var name
        consumeNewline();
        return fe;
    }

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
ExprPtr Parser::parseExpr() {
    if (check(TokKind::From)) return parseQuery();
    return parseOr();
}

ExprPtr Parser::parseQuery() {
    int line = peek().line;
    advance();                                  // consume From

    Token name = expect(TokKind::Ident, "query variable name");

    // 'In' is not a reserved keyword (kept that way so simple identifiers
    // don't break) — accept it as an identifier here.
    if (check(TokKind::Ident) && peek().lexeme.compare("In", Qt::CaseInsensitive) == 0) {
        advance();
    } else {
        errorAt(peek(), "expected 'In' after From <var>");
    }

    auto q = std::make_shared<QueryExpr>(line);
    q->sourceVar = name.lexeme;
    q->source    = parseOr();                   // not parseExpr → no nested queries

    while (true) {
        if (match(TokKind::Where)) {
            q->whereExpr = parseOr();
        } else if (match(TokKind::OrderBy)) {
            do {
                QueryExpr::OrderByClause c;
                c.expr = parseOr();
                if      (match(TokKind::Descending)) c.descending = true;
                else if (match(TokKind::Ascending))  c.descending = false;
                q->orderBy.append(c);
            } while (match(TokKind::Comma));
        } else if (match(TokKind::Select)) {
            q->selectExpr = parseOr();
        } else if (match(TokKind::Take)) {
            q->takeExpr = parseOr();
        } else {
            break;
        }
    }
    return q;
}

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
            k == TokKind::Gt || k == TokKind::GtEq ||
            k == TokKind::Is || k == TokKind::IsNot) {
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
            // Allow:  obj.method(args)  immediately after the property.
            if (check(TokKind::LParen)) {
                advance();
                QVector<ExprPtr> args = parseArgs();
                expect(TokKind::RParen, "')'");
                // Encode method call as a CallExpr whose name is "@member"
                // and whose first arg is the receiver MemberExpr.  The
                // interpreter recognises this idiom.
                auto recv = e;
                auto call = std::make_shared<CallExpr>(p.line, "@member", QVector<ExprPtr>{recv});
                for (const auto &a : args) call->args.append(a);
                e = call;
            }
            continue;
        }
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
