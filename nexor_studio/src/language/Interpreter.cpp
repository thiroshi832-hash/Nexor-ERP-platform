#include "Interpreter.h"
#include <QDateTime>
#include <stdexcept>
#include <cmath>
#if defined(NEXOR_HAS_WIDGETS)
#  include <QMessageBox>
#endif

namespace nx {

// ─── Construction ────────────────────────────────────────────────────────
Interpreter::Interpreter()
    : m_globals(std::make_shared<Environment>()) {
    installBuiltins();
}

void Interpreter::registerSheet(const SheetSchema &schema) {
    m_store.registerSheet(schema);
}

void Interpreter::registerProcess(const QString &name, ProcessRunner runner) {
    if (name.isEmpty()) return;
    m_processes.insert(name.toLower(), std::move(runner));
}

bool Interpreter::hasProcess(const QString &name) const {
    return m_processes.contains(name.toLower());
}

Value Interpreter::getVar(const QString &name) const {
    if (!m_varStore) return Value();
    return m_varStore->value(name.toLower(), Value());
}

void Interpreter::setVar(const QString &name, const Value &v) {
    if (!m_varStore) return;
    m_varStore->insert(name.toLower(), v);
}

// Returns a Value carrying the process name as a simple String holder.
// kind="Process" + the lowercase-id stored as the object handle (we just use
// a heap-allocated QString shared_ptr).  Member dispatch finds it via the
// runner table on the Interpreter.
static Value makeProcessRefValue(const QString &name) {
    auto holder = std::make_shared<QString>(name);
    return Value::object(holder, "Process");
}

// Pulls the sheet handle for a name, if registered.
static Value makeSheetRefValue(const QString &id, EntityStore *store) {
    auto ref = std::make_shared<SheetRef>();
    ref->sheetId = id;
    ref->store   = store;
    return Value::object(ref, "Sheet");
}

static std::shared_ptr<Entity> entityHandle(const Value &v) {
    if (v.kind() != Value::Object) return nullptr;
    if (v.objectKind() != "Entity") return nullptr;
    return std::static_pointer_cast<Entity>(v.objectHandle());
}

static std::shared_ptr<SheetRef> sheetHandle(const Value &v) {
    if (v.kind() != Value::Object) return nullptr;
    if (v.objectKind() != "Sheet") return nullptr;
    return std::static_pointer_cast<SheetRef>(v.objectHandle());
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

    // RunsOn dispatch — the heart of split execution.
    if (s->isServerOnly() && m_hostRole == HostRole::Client) {
        if (m_rpcBridge) return m_rpcBridge(s->name, args);
        m_lastError = QString("[ServerOnly] '%1' called from client without an "
                              "RPC bridge installed.").arg(s->name);
        if (m_errorOut) m_errorOut(m_lastError);
        return Value();
    }
    if (s->isClientOnly() && m_hostRole == HostRole::Server) {
        m_lastError = QString("[ClientOnly] '%1' cannot run on the server.").arg(s->name);
        if (m_errorOut) m_errorOut(m_lastError);
        return Value();
    }

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
    case Stmt::MemberAssignStmt: {
        auto *m = static_cast<MemberAssignStatement*>(s);
        Value obj = evalExpr(m->object.get(), env);
        Value v   = evalExpr(m->value.get(),  env);
        setMember(obj, m->property, v);
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
    case Stmt::ForEachStmt: {
        auto *f = static_cast<ForEachStatement*>(s);
        Value coll = evalExpr(f->collection.get(), env);
        if (coll.kind() != Value::List) {
            // Not a list — silently no-op.  Future: iterate dict keys, etc.
            return;
        }
        env->define(f->var, Value());
        try {
            for (const auto &item : coll.listRef()) {
                env->assign(f->var, item);
                execBlock(f->body, env);
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
        QString lo = v->name.toLower();
        // 1. Local / global variable wins.
        if (env->has(v->name)) return env->get(v->name);
        // 2. The magic "Form" name (only meaningful when a FormRunner has
        //    installed bridge callbacks on this interpreter).
        if (lo == "form" && (m_formHandler || m_formReader)) {
            return Value::object(std::shared_ptr<void>(), "Form");
        }
        // 2b. The magic "Vars" name — process-level shared dictionary.
        //     Inert (Empty member access) when no var store is installed.
        if (lo == "vars") {
            return Value::object(std::shared_ptr<void>(), "Vars");
        }
        // 3. Registered sheet name (Customer, Order, …) → SheetRef value.
        if (m_store.hasSheet(v->name)) return makeSheetRefValue(v->name, &m_store);
        // 3b. Registered process name (OrderApproval, …) → ProcessRef value.
        if (m_processes.contains(lo)) return makeProcessRefValue(v->name);
        // 4. No-arg user-defined sub.
        if (m_subs.contains(lo))     return call(v->name, {});
        // 5. No-arg built-in.
        if (m_builtins.contains(lo)) return m_builtins.value(lo)(*this, {});
        return Value();             // implicitly empty
    }
    case Expr::Unary:    return evalUnary  (static_cast<UnaryExpr*>  (e), env);
    case Expr::Binary:   return evalBinary (static_cast<BinaryExpr*> (e), env);
    case Expr::Logical:  return evalLogical(static_cast<LogicalExpr*>(e), env);
    case Expr::Call:     return evalCall   (static_cast<CallExpr*>   (e), env);
    case Expr::Member: {
        auto *m = static_cast<MemberExpr*>(e);
        Value obj = evalExpr(m->object.get(), env);
        return getMember(obj, m->property);
    }
    case Expr::Query:
        return evalQuery(static_cast<QueryExpr*>(e), env);
    }
    return Value();
}

// ─── Query (LINQ-style) ─────────────────────────────────────────────────
//
// Evaluation strategy is straightforward in-memory: pull the source into a
// list, run Where as a filter, sort by OrderBy keys, project via Select,
// then apply Take.  The optimiser that pushes filters into SQL will land
// in Phase 5b once we have more demanding workloads.
Value Interpreter::evalQuery(QueryExpr *q, std::shared_ptr<Environment> env) {
    Value src = evalExpr(q->source.get(), env);

    // Expand a SheetRef source automatically into the full list of rows.
    if (src.kind() == Value::Object && src.objectKind() == "Sheet") {
        src = callMember(src, "All", {});
    }
    if (src.kind() != Value::List) {
        return Value::list({});      // not iterable
    }

    QVector<Value> all = src.listRef();

    // 1. Where ───────────────────────────────────────────────────────────
    QVector<Value> filtered;
    filtered.reserve(all.size());
    for (const Value &item : all) {
        auto child = std::make_shared<Environment>(env);
        child->define(q->sourceVar, item);
        if (q->whereExpr) {
            Value c = evalExpr(q->whereExpr.get(), child);
            if (!c.toBool()) continue;
        }
        filtered.append(item);
    }

    // 2. OrderBy ─────────────────────────────────────────────────────────
    if (!q->orderBy.isEmpty()) {
        // Stable sort with successive keys — but std::sort isn't guaranteed
        // stable; use std::stable_sort.
        std::stable_sort(filtered.begin(), filtered.end(),
            [&](const Value &a, const Value &b) {
                for (const auto &c : q->orderBy) {
                    auto envA = std::make_shared<Environment>(env);
                    envA->define(q->sourceVar, a);
                    auto envB = std::make_shared<Environment>(env);
                    envB->define(q->sourceVar, b);
                    Value va = evalExpr(c.expr.get(), envA);
                    Value vb = evalExpr(c.expr.get(), envB);
                    int cmp = Value::compare(va, vb);
                    if (cmp != 0) return c.descending ? cmp > 0 : cmp < 0;
                }
                return false;
            });
    }

    // 3. Take ────────────────────────────────────────────────────────────
    if (q->takeExpr) {
        Value tv = evalExpr(q->takeExpr.get(), env);
        qint64 n = tv.toLong();
        if (n < 0) n = 0;
        if (n < filtered.size()) filtered.resize(int(n));
    }

    // 4. Select ──────────────────────────────────────────────────────────
    if (q->selectExpr) {
        QVector<Value> projected;
        projected.reserve(filtered.size());
        for (const Value &item : filtered) {
            auto child = std::make_shared<Environment>(env);
            child->define(q->sourceVar, item);
            projected.append(evalExpr(q->selectExpr.get(), child));
        }
        return Value::list(std::move(projected));
    }
    return Value::list(std::move(filtered));
}

// ─── Member access + sheet/entity methods ───────────────────────────────
Value Interpreter::getMember(const Value &obj, const QString &prop) {
    // Form bridge takes priority for kind=="Form" objects.
    if (obj.kind() == Value::Object && obj.objectKind() == "Form") {
        return m_formReader ? m_formReader(prop) : Value();
    }
    // Vars dictionary — Vars.<name> reads from the per-process var store.
    if (obj.kind() == Value::Object && obj.objectKind() == "Vars") {
        return getVar(prop);
    }
    // Entity field access
    if (auto e = entityHandle(obj)) {
        return e->get(prop);
    }
    // Sheet method-as-property doesn't make sense; user must call .New()/etc.
    return Value();
}

void Interpreter::setMember(const Value &obj, const QString &prop, const Value &v) {
    if (obj.kind() == Value::Object && obj.objectKind() == "Form") {
        if (m_formWriter) m_formWriter(prop, v);
        return;
    }
    if (obj.kind() == Value::Object && obj.objectKind() == "Vars") {
        setVar(prop, v);
        return;
    }
    if (auto e = entityHandle(obj)) {
        e->set(prop, v);
        return;
    }
    // Silently ignore for now.
}

Value Interpreter::callMember(const Value &obj, const QString &name,
                              const QVector<Value> &args) {
    // Form bridge — handle Form.Save / Form.Load / Form.New / etc.
    if (obj.kind() == Value::Object && obj.objectKind() == "Form") {
        return m_formHandler ? m_formHandler(name, args) : Value();
    }
    // Process bridge — handle <ProcessName>.Start() and friends.
    if (obj.kind() == Value::Object && obj.objectKind() == "Process") {
        auto holder = std::static_pointer_cast<QString>(obj.objectHandle());
        if (!holder) return Value();
        auto it = m_processes.find(holder->toLower());
        if (it == m_processes.end()) return Value();
        if (name.compare("Start", Qt::CaseInsensitive) == 0) {
            return it.value()(args);
        }
        return Value();
    }
    QString lo = name.toLower();

    // ── Sheet methods: New / Find / All / Count / Delete
    if (auto sr = sheetHandle(obj)) {
        EntityTable *t = sr->store ? sr->store->table(sr->sheetId) : nullptr;
        if (!t) return Value();

        if (lo == "new") {
            auto ent = t->create();
            return Value::object(ent, "Entity");
        }
        if (lo == "find") {
            qint64 id = args.isEmpty() ? 0 : args.first().toLong();
            auto ent = t->find(id);
            if (!ent) return Value::nothing();
            return Value::object(ent, "Entity");
        }
        if (lo == "all") {
            QVector<Value> rows;
            for (const auto &e : t->all())
                rows.append(Value::object(e, "Entity"));
            return Value::list(std::move(rows));
        }
        if (lo == "count") {
            return Value::integer(t->all().size());
        }
        if (lo == "delete") {
            qint64 id = args.isEmpty() ? 0 : args.first().toLong();
            return Value::boolean(t->remove(id));
        }
        return Value();
    }

    // ── Entity methods: Save / Delete
    if (auto e = entityHandle(obj)) {
        EntityTable *t = m_store.table(e->sheetId());
        if (!t) return Value();
        if (lo == "save")   { return Value::boolean(t->save(e)); }
        if (lo == "delete") { return Value::boolean(t->remove(e->id())); }
        // Otherwise treat as a property read (e.g., user wrote   e.Name())
        return e->get(name);
    }
    return Value();
}

Value Interpreter::evalCall(CallExpr *c, std::shared_ptr<Environment> env) {
    // Special-case: parser-synthesised "@member" call → obj.method(args).
    if (c->name == "@member" && !c->args.isEmpty()) {
        // First "arg" is actually the MemberExpr receiver carrying the
        // property name.  Evaluate the underlying object, then dispatch.
        ExprPtr recvExpr = c->args.first();
        if (recvExpr && recvExpr->kind == Expr::Member) {
            auto *me = static_cast<MemberExpr*>(recvExpr.get());
            Value obj = evalExpr(me->object.get(), env);
            QVector<Value> args;
            args.reserve(c->args.size() - 1);
            for (int i = 1; i < c->args.size(); ++i)
                args.append(evalExpr(c->args[i].get(), env));
            return callMember(obj, me->property, args);
        }
    }

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
    // Identity comparisons.  Nothing is the canonical "no value", so the
    // common idiom is `x Is Nothing` (true if x is Empty / has no object
    // handle) and `x IsNot Nothing` (the negation).  For two Object values
    // we compare the underlying handle pointers - same handle means same
    // entity / sheet / form.  Everything else falls back to the structural
    // equality used by '='.
    case TokKind::Is:
    case TokKind::IsNot: {
        bool same;
        if (l.kind() == Value::Empty || r.kind() == Value::Empty) {
            same = (l.kind() == Value::Empty && r.kind() == Value::Empty);
        } else if (l.kind() == Value::Object && r.kind() == Value::Object) {
            same = (l.objectHandle().get() == r.objectHandle().get());
        } else {
            same = (Value::compare(l, r) == 0);
        }
        return Value::boolean(b->op == TokKind::Is ? same : !same);
    }
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
    registerBuiltin("MsgBox", [](Interpreter &ip, const QVector<Value> &a) {
        QString msg = argText(a, 0);
#if defined(NEXOR_HAS_WIDGETS)
        QMessageBox::information(nullptr, "Nexor", msg);
#else
        if (ip.m_output) ip.m_output("MsgBox: " + msg);
#endif
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
