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

    // Throws a runtime error (caught at top-level call()).
    [[noreturn]] void runtimeError(int line, const QString &msg);

    // Internal control-flow exceptions
    struct ReturnSignal { Value value; };
    struct ExitSignal   { ExitStatement::What what; };

    // Globals + registered routines
    std::shared_ptr<Environment>    m_globals;
    QHash<QString, SubPtr>          m_subs;          // case-insensitive (toLower)
    QHash<QString, BuiltinFn>       m_builtins;

    OutputCallback m_output;
    OutputCallback m_errorOut;
    QString        m_lastError;
    QString        m_currentUnit;
};

} // namespace nx

#endif // NEXOR_STUDIO_LANG_INTERPRETER_H
