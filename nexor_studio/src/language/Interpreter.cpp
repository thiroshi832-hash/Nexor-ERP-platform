#include "Interpreter.h"
#include <QDateTime>
#include <QMessageBox>
#include <stdexcept>
#include <cmath>

namespace nx {

// ─── Construction ────────────────────────────────────────────────────────
Interpreter::Interpreter()
    : m_globals(std::make_shared<Environment>()) {
    installBuiltins();
}

// ─── Load a parsed Program ───────────────────────────────────────────────
bool Interpreter::load(const Program &p, const QString &unitName) {
    m_currentUnit = unitName;
    for (const auto &s : p.subs)
        m_subs.insert(s->name.toLower(), s);

    // Run module-level statements (Dim, etc.) once to populate globals.
    try {
        execBlock(p.topLevel, m_globals);
    } catch (const ReturnSignal&) {
        // a Return outside a sub — ignore.
    } catch (const std::exception &e) {
        m_lastError = QString::fromUtf8(e.what());
        if (m_errorOut) m_errorOut(m_lastError);
        return false;
    }
    return true;
}

bool Interpreter::hasSub(const QString &name) const {
    return m_subs.contains(name.toLower());
}

void Interpreter::registerBuiltin(const QString &name, BuiltinFn fn) {
    m_builtins.insert(name.toLower(), std::move(fn));
}

// ─── Top-level call ──────────────────────────────────────────────────────
Value Interpreter::call(const QString &name, const QVector<Value> &args) {
    m_lastError.clear();
    auto it = m_subs.find(name.toLower());
    if (it == m_subs.end()) {
        // Maybe a builtin
        auto bi = m_builtins.find(name.toLower());
        if (bi != m_builtins.end()) return bi.value()(*this, args);
        // Otherwise — silent no-op (e.g., btnX_Click without a handler)
        return Value();
    }
    SubPtr s = it.value();

    // Build a child env with the parameters bound.
    auto env = std::make_shared<Environment>(m_globals);
    int n = qMin(s->params.size(), args.size());
    for (int i = 0; i < n; ++i)
        env->define(s->params[i], args[i]);
    // Any unsupplied params default to Empty.
    for (int i = n; i < s->params.size(); ++i)
        env->define(s->params[i], Value());

    try {
        execBlock(s->body, env);
    } catch (const ReturnSignal &r) {
        return r.value;
    } catch (const ExitSignal&) {
        // Exit Sub / Exit Function — just stop.
    } catch (const std::exception &e) {
        m_lastError = QString::fromUtf8(e.what());
        if (m_errorOut) m_errorOut(m_lastError);
    }
    return Value();
}

// ─── Statement execution ─────────────────────────────────────────────────
void Interpreter::execBlock(const QVector<StmtPtr> &b,
                            std::shared_ptr<Environment> env) {
    for (const auto &s : b) execStmt(s.get(), env);
}

void Interpreter::execStmt(Stmt *s, std::shared_ptr<Environment> env) {
    if (!s) return;
    switch (s->kind) {
    case Stmt::ExprStmt: {
        auto *es = static_cast<ExprStatement*>(s);
        evalExpr(es->expr.get(), env);
        return;
    }
    case Stmt::DimStmt: {
        auto *d = static_cast<DimStatement*>(s);
        Value v = d->initializer ? evalExpr(d->initializer.get(), env) : Value();
        env->define(d->name, v);
        return;
    }
    case Stmt::AssignStmt: {
        auto *a = static_cast<AssignStatement*>(s);
        Value v = evalExpr(a->value.get(), env);
        env->assign(a->name, v);
        return;
    }
    case Stmt::IfStmt: {
        auto *is = static_cast<IfStatement*>(s);
        for (const auto &b : is->branches) {
            if (evalExpr(b.cond.get(), env).toBool()) {
                execBlock(b.body, env);
                return;
            }
        }
        execBlock(is->elseBody, env);
        return;
    }
    case Stmt::WhileStmt: {
        auto *w = static_cast<WhileStatement*>(s);
        try {
            while (evalExpr(w->cond.get(), env).toBool()) {
                execBlock(w->body, env);
            }
        } catch (const ExitSignal &x) {
            if (x.what != ExitStatement::ExitWhile && x.what != ExitStatement::ExitDo)
                throw;
        }
        return;
    }
    case Stmt::ForStmt: {
        auto *f = static_cast<ForStatement*>(s);
        Value sv = evalExpr(f->start.get(), env);
        Value ev = evalExpr(f->end  .get(), env);
        Value stp = f->step ? evalExpr(f->step.get(), env) : Value::integer(1);
        bool real = sv.kind() == Value::Double || ev.kind() == Value::Double
                                              || stp.kind() == Value::Double;
        env->define(f->var, sv);
        try {
            if (real) {
                double i = sv.toDouble(), e = ev.toDouble(), st = stp.toDouble();
                while ((st >= 0 ? i <= e : i >= e)) {
                    env->assign(f->var, Value::real(i));
                    execBlock(f->body, env);
                    i += st;
                }
            } else {
                qint64 i = sv.toLong(), e = ev.toLong(), st = stp.toLong();
                if (st == 0) st = 1;
                while ((st >= 0 ? i <= e : i >= e)) {
                    env->assign(f->var, Value::integer(i));
                    execBlock(f->body, env);
                    i += st;
                }
            }
        } catch (const ExitSignal &x) {
            if (x.what != ExitStatement::ExitFor) throw;
        }
        return;
    }
    case Stmt::ReturnStmt: {
        auto *r = static_cast<ReturnStatement*>(s);
        throw ReturnSignal{ r->value ? evalExpr(r->value.get(), env) : Value() };
    }
    case Stmt::ExitStmt: {
        auto *x = static_cast<ExitStatement*>(s);
        throw ExitSignal{ x->what };
    }
    case Stmt::PrintStmt: {
        auto *p = static_cast<PrintStatement*>(s);
        QString text = p->expr ? evalExpr(p->expr.get(), env).toText() : QString();
        if (m_output) m_output(text);
        return;
    }
    case Stmt::BlockStmt: {
        auto *b = static_cast<BlockStatement*>(s);
        execBlock(b->stmts, env);
        return;
    }
    }
}

// ─── Expression evaluation ───────────────────────────────────────────────
Value Interpreter::evalExpr(Expr *e, std::shared_ptr<Environment> env) {
    if (!e) return Value();
    switch (e->kind) {
    case Expr::Literal:  return static_cast<LiteralExpr*>(e)->value;
    case Expr::Variable: {
        auto *v = static_cast<VariableExpr*>(e);
        // A bare identifier without parens may be a no-arg sub call (VB style).
        QString lo = v->name.toLower();
        if (m_subs.contains(lo)) return call(v->name, {});
        if (m_builtins.contains(lo)) return m_builtins.value(lo)(*this, {});
        if (env->has(v->name)) return env->get(v->name);
        return Value();             // implicitly empty
    }
    case Expr::Unary:    return evalUnary  (static_cast<UnaryExpr*>  (e), env);
    case Expr::Binary:   return evalBinary (static_cast<BinaryExpr*> (e), env);
    case Expr::Logical:  return evalLogical(static_cast<LogicalExpr*>(e), env);
    case Expr::Call:     return evalCall   (static_cast<CallExpr*>   (e), env);
    case Expr::Member: {
        // Reserved for future entity / form member access.  Returns Empty
        // for now so user code referencing form properties doesn't crash.
        return Value();
    }
    }
    return Value();
}

Value Interpreter::evalCall(CallExpr *c, std::shared_ptr<Environment> env) {
    QVector<Value> args;
    args.reserve(c->args.size());
    for (const auto &a : c->args) args.append(evalExpr(a.get(), env));

    QString lo = c->name.toLower();
    auto it = m_subs.find(lo);
    if (it != m_subs.end()) return call(c->name, args);
    auto bi = m_builtins.find(lo);
    if (bi != m_builtins.end()) return bi.value()(*this, args);

    // Unknown — soft-fail with empty so missing handlers don't blow up.
    return Value();
}

Value Interpreter::evalBinary(BinaryExpr *b, std::shared_ptr<Environment> env) {
    Value l = evalExpr(b->left.get(),  env);
    Value r = evalExpr(b->right.get(), env);
    switch (b->op) {
    case TokKind::Plus:      return Value::add(l, r);
    case TokKind::Minus:     return Value::sub(l, r);
    case TokKind::Star:      return Value::mul(l, r);
    case TokKind::Slash:     return Value::div(l, r);
    case TokKind::Mod:       return Value::mod(l, r);
    case TokKind::Ampersand: return Value::concat(l, r);
    case TokKind::Eq:        return Value::boolean(Value::compare(l, r) == 0);
    case TokKind::NotEq:     return Value::boolean(Value::compare(l, r) != 0);
    case TokKind::Lt:        return Value::boolean(Value::compare(l, r) <  0);
    case TokKind::LtEq:      return Value::boolean(Value::compare(l, r) <= 0);
    case TokKind::Gt:        return Value::boolean(Value::compare(l, r) >  0);
    case TokKind::GtEq:      return Value::boolean(Value::compare(l, r) >= 0);
    default:
        runtimeError(b->line, QString("unsupported binary operator '%1'")
                                .arg(Token::kindName(b->op)));
    }
}

Value Interpreter::evalLogical(LogicalExpr *l, std::shared_ptr<Environment> env) {
    Value lv = evalExpr(l->left.get(), env);
    bool  lb = lv.toBool();
    if (l->op == TokKind::Or  &&  lb) return Value::boolean(true);
    if (l->op == TokKind::And && !lb) return Value::boolean(false);
    Value rv = evalExpr(l->right.get(), env);
    return Value::boolean(rv.toBool());
}

Value Interpreter::evalUnary(UnaryExpr *u, std::shared_ptr<Environment> env) {
    Value v = evalExpr(u->expr.get(), env);
    switch (u->op) {
    case TokKind::Minus:
        if (v.kind() == Value::Double) return Value::real(-v.toDouble());
        return Value::integer(-v.toLong());
    case TokKind::Not:
        return Value::boolean(!v.toBool());
    default:
        runtimeError(u->line, "unsupported unary operator");
    }
}

[[noreturn]] void Interpreter::runtimeError(int line, const QString &msg) {
    QString full = QString("[%1:%2] %3").arg(m_currentUnit).arg(line).arg(msg);
    throw std::runtime_error(full.toStdString());
}

// ─── Built-in functions ──────────────────────────────────────────────────
static QString argText (const QVector<Value> &a, int i) { return i < a.size() ? a[i].toText()  : QString(); }
static qint64  argLong (const QVector<Value> &a, int i) { return i < a.size() ? a[i].toLong()  : 0; }
static double  argDbl  (const QVector<Value> &a, int i) { return i < a.size() ? a[i].toDouble(): 0; }

void Interpreter::installBuiltins() {
    // Output
    registerBuiltin("Print", [](Interpreter &ip, const QVector<Value> &a) {
        QString out;
        for (int i = 0; i < a.size(); ++i) { if (i) out += " "; out += a[i].toText(); }
        if (ip.m_output) ip.m_output(out);
        return Value();
    });
    registerBuiltin("MsgBox", [](Interpreter&, const QVector<Value> &a) {
        QMessageBox::information(nullptr, "Nexor",
            argText(a, 0));
        return Value();
    });
    registerBuiltin("InputBox", [](Interpreter&, const QVector<Value> &a) {
        // Use a default for now so we don't block scripts in headless contexts.
        return Value::text(argText(a, 1));
    });

    // Type conversion
    registerBuiltin("CStr",  [](Interpreter&, const QVector<Value> &a){ return Value::text   (argText(a,0)); });
    registerBuiltin("CInt",  [](Interpreter&, const QVector<Value> &a){ return Value::integer(argLong(a,0)); });
    registerBuiltin("CLng",  [](Interpreter&, const QVector<Value> &a){ return Value::integer(argLong(a,0)); });
    registerBuiltin("CDbl",  [](Interpreter&, const QVector<Value> &a){ return Value::real   (argDbl (a,0)); });
    registerBuiltin("CBool", [](Interpreter&, const QVector<Value> &a){
        return Value::boolean(a.isEmpty() ? false : a[0].toBool());
    });

    // String
    registerBuiltin("Len",   [](Interpreter&, const QVector<Value> &a){
        return Value::integer(argText(a,0).size());
    });
    registerBuiltin("UCase", [](Interpreter&, const QVector<Value> &a){
        return Value::text(argText(a,0).toUpper());
    });
    registerBuiltin("LCase", [](Interpreter&, const QVector<Value> &a){
        return Value::text(argText(a,0).toLower());
    });
    registerBuiltin("Trim",  [](Interpreter&, const QVector<Value> &a){
        return Value::text(argText(a,0).trimmed());
    });
    registerBuiltin("Left",  [](Interpreter&, const QVector<Value> &a){
        return Value::text(argText(a,0).left(int(argLong(a,1))));
    });
    registerBuiltin("Right", [](Interpreter&, const QVector<Value> &a){
        return Value::text(argText(a,0).right(int(argLong(a,1))));
    });
    registerBuiltin("Mid",   [](Interpreter&, const QVector<Value> &a){
        QString s = argText(a,0);
        int start = int(argLong(a,1)) - 1;            // VB is 1-based
        if (start < 0) start = 0;
        if (a.size() >= 3) return Value::text(s.mid(start, int(argLong(a,2))));
        return Value::text(s.mid(start));
    });

    // Math
    registerBuiltin("Abs",   [](Interpreter&, const QVector<Value> &a){
        return Value::real(std::fabs(argDbl(a,0)));
    });
    registerBuiltin("Int",   [](Interpreter&, const QVector<Value> &a){
        return Value::integer(qint64(std::floor(argDbl(a,0))));
    });
    registerBuiltin("Round", [](Interpreter&, const QVector<Value> &a){
        return Value::real(std::round(argDbl(a,0)));
    });

    // Date/Time
    registerBuiltin("Now",   [](Interpreter&, const QVector<Value>&){
        return Value::text(QDateTime::currentDateTime().toString(Qt::ISODate));
    });
}

} // namespace nx
