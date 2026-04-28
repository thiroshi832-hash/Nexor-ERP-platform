// =============================================================================
// Interpreter — tree-walks an AST produced by Parser.
//
// One Interpreter holds the global environment, the registered Subs/Functions,
// and a callback for Print output.  Use load() to register a parsed Program
// (e.g., the activity's Sub Main + globals); call sub(name) to invoke a Sub.
//
// Form-event glue lives outside this class: FormRunner builds an
// Interpreter, registers each form's parsed code, and on QPushButton::clicked
// asks the Interpreter to call e.g. "btnSave_Click".
// =============================================================================
#ifndef NEXOR_STUDIO_LANG_INTERPRETER_H
#define NEXOR_STUDIO_LANG_INTERPRETER_H

#include "Ast.h"
#include "Value.h"
#include "Environment.h"
#include "EntityStore.h"
#include <QHash>
#include <QString>
#include <functional>
#include <memory>

namespace nx {

class Interpreter {
public:
    using OutputCallback = std::function<void(const QString &)>;
    using BuiltinFn      = std::function<Value(Interpreter&, const QVector<Value>&)>;

    Interpreter();

    // Hook for Print output (and runtime error reporting).
    void setOutput(OutputCallback cb)      { m_output = std::move(cb); }
    void setError (OutputCallback cb)      { m_errorOut = std::move(cb); }
    OutputCallback output() const          { return m_output; }
    OutputCallback errorOut() const        { return m_errorOut; }

    // Register a parsed source unit (.aba or .frm <Code>).
    // Top-level Dim statements run immediately to initialise globals.
    bool load(const Program &p, const QString &unitName);

    // Returns true if a Sub/Function with that name has been loaded.
    bool hasSub(const QString &name) const;

    // Call a top-level Sub or Function by name.  Pass any args; the function's
    // Return value is returned (Sub returns Empty).
    Value call(const QString &name, const QVector<Value> &args = {});

    // Manual stdlib install.
    void installBuiltins();
    void registerBuiltin(const QString &name, BuiltinFn fn);

    // Last runtime error (set by call() if execution raised one).
    QString lastError() const { return m_lastError; }
    bool    hadError()  const { return !m_lastError.isEmpty(); }

    // Access the global environment (used by FormRunner to bind widgets).
    std::shared_ptr<Environment> globals() const { return m_globals; }

    // Sheets / entities
    EntityStore *entityStore() { return &m_store; }
    void         registerSheet(const SheetSchema &schema);

    // Processes — exposes  <ProcessName>.Start()  as a runtime callable.
    // The host (FormRunner / MainWindow / Activity) supplies the runner
    // callback so the Interpreter does not depend on ProcessEngine.
    using ProcessRunner = std::function<Value(const QVector<Value> &args)>;
    void registerProcess(const QString &name, ProcessRunner runner);
    bool hasProcess(const QString &name) const;

    // Host role — drives the RunsOn dispatch in call().
    //   "client"  : ServerOnly subs are routed through the RPC bridge.
    //   "server"  : ServerOnly subs run locally; ClientOnly subs error.
    //   "either"  : everything runs locally (default; Studio + tests).
    enum class HostRole { Either, Client, Server };
    void     setHostRole(HostRole r) { m_hostRole = r; }
    HostRole hostRole() const        { return m_hostRole; }

    // RPC bridge — installed by the client host (Flux) so that calls to
    // a [Activity(RunsOn := ServerOnly)] sub get forwarded to Core.  The
    // bridge takes (subName, args) and returns the result Value.
    using RpcBridge = std::function<Value(const QString &subName,
                                          const QVector<Value> &args)>;
    void setRpcBridge(RpcBridge b) { m_rpcBridge = std::move(b); }

    // Process-level shared variables — the magic identifier "Vars" resolves
    // to a dictionary that step bodies can read and write:
    //     Vars.Total = 42
    //     Print Vars.Total
    // The host (ProcessEngine) installs a backing store; if no store has
    // been installed, Vars is silently inert (so a stray Vars.X in a Form
    // body doesn't crash).
    using VarStore = QHash<QString, Value>;            // case-insensitive keys
    void  setVarStore(VarStore *store) { m_varStore = store; }
    bool  hasVarStore() const          { return m_varStore != nullptr; }
    Value getVar(const QString &name) const;
    void  setVar(const QString &name, const Value &v);

    // Form bridge — FormRunner installs these so user code that says
    //   Form.Save()        Form.Load(id)        Form.Current.Name
    // dispatches into the live form's data binding.
    using FormHandler = std::function<Value(const QString &method,
                                            const QVector<Value> &args)>;
    using FormReader  = std::function<Value(const QString &prop)>;
    using FormWriter  = std::function<void (const QString &prop, const Value &v)>;
    void setFormHandler(FormHandler h) { m_formHandler = std::move(h); }
    void setFormReader (FormReader  r) { m_formReader  = std::move(r); }
    void setFormWriter (FormWriter  w) { m_formWriter  = std::move(w); }

    // Resolves a member access:  obj.prop  →  Value
    // Used by the parser-level MemberExpr and by member assignment.
    Value getMember(const Value &obj, const QString &prop);
    void  setMember(const Value &obj, const QString &prop, const Value &v);
    Value callMember(const Value &obj, const QString &name,
                     const QVector<Value> &args);

private:
    // Statement execution ─────────────────────────────────────────────
    void execStmt (Stmt *s, std::shared_ptr<Environment> env);
    void execBlock(const QVector<StmtPtr> &b, std::shared_ptr<Environment> env);

    // Expression evaluation
    Value evalExpr(Expr *e, std::shared_ptr<Environment> env);
    Value evalCall(CallExpr *c, std::shared_ptr<Environment> env);
    Value evalBinary (BinaryExpr  *b, std::shared_ptr<Environment> env);
    Value evalLogical(LogicalExpr *l, std::shared_ptr<Environment> env);
    Value evalUnary  (UnaryExpr   *u, std::shared_ptr<Environment> env);
    Value evalQuery  (class QueryExpr *q, std::shared_ptr<Environment> env);

    // Throws a runtime error (caught at top-level call()).
    [[noreturn]] void runtimeError(int line, const QString &msg);

    // Internal control-flow exceptions
    struct ReturnSignal { Value value; };
    struct ExitSignal   { ExitStatement::What what; };

    // Globals + registered routines
    std::shared_ptr<Environment>    m_globals;
    QHash<QString, SubPtr>          m_subs;          // case-insensitive (toLower)
    QHash<QString, BuiltinFn>       m_builtins;
    QHash<QString, ProcessRunner>   m_processes;    // case-insensitive
    VarStore                       *m_varStore { nullptr };
    EntityStore                     m_store;

    OutputCallback m_output;
    OutputCallback m_errorOut;
    QString        m_lastError;
    QString        m_currentUnit;

    FormHandler    m_formHandler;
    FormReader     m_formReader;
    FormWriter     m_formWriter;
    HostRole       m_hostRole { HostRole::Either };
    RpcBridge      m_rpcBridge;
};

} // namespace nx

#endif // NEXOR_STUDIO_LANG_INTERPRETER_H
