// =============================================================================
// Ast — discriminated AST nodes for the Nexor procedural subset.
//
// Nodes are reference-counted via shared_ptr.  Each node carries a source
// line number so the interpreter can report errors with location.
// =============================================================================
#ifndef NEXOR_STUDIO_LANG_AST_H
#define NEXOR_STUDIO_LANG_AST_H

#include "Value.h"
#include "Token.h"
#include <QString>
#include <QVector>
#include <memory>

namespace nx {

// ── Forward declarations ────────────────────────────────────────────────
class Expr;     using ExprPtr = std::shared_ptr<Expr>;
class Stmt;     using StmtPtr = std::shared_ptr<Stmt>;
class SubDecl;  using SubPtr  = std::shared_ptr<SubDecl>;

// =============================================================================
// Expression kinds
// =============================================================================
class Expr {
public:
    enum Kind {
        Literal,
        Variable,
        Unary,
        Binary,
        Logical,
        Call,
        Member,         // a.b   (reserved for future entity support)
    };

    explicit Expr(Kind k, int line) : kind(k), line(line) {}
    virtual ~Expr() = default;

    Kind kind;
    int  line;
};

class LiteralExpr : public Expr {
public:
    LiteralExpr(int line, const Value &v) : Expr(Literal, line), value(v) {}
    Value value;
};

class VariableExpr : public Expr {
public:
    VariableExpr(int line, QString n) : Expr(Variable, line), name(std::move(n)) {}
    QString name;
};

class UnaryExpr : public Expr {
public:
    UnaryExpr(int line, TokKind op, ExprPtr e) : Expr(Unary, line), op(op), expr(std::move(e)) {}
    TokKind op;
    ExprPtr expr;
};

class BinaryExpr : public Expr {
public:
    BinaryExpr(int line, TokKind op, ExprPtr l, ExprPtr r)
        : Expr(Binary, line), op(op), left(std::move(l)), right(std::move(r)) {}
    TokKind op;
    ExprPtr left, right;
};

class LogicalExpr : public Expr {
public:
    LogicalExpr(int line, TokKind op, ExprPtr l, ExprPtr r)
        : Expr(Logical, line), op(op), left(std::move(l)), right(std::move(r)) {}
    TokKind op;             // And, Or
    ExprPtr left, right;
};

class CallExpr : public Expr {
public:
    CallExpr(int line, QString n, QVector<ExprPtr> a)
        : Expr(Call, line), name(std::move(n)), args(std::move(a)) {}
    QString          name;
    QVector<ExprPtr> args;
};

class MemberExpr : public Expr {
public:
    MemberExpr(int line, ExprPtr obj, QString prop)
        : Expr(Member, line), object(std::move(obj)), property(std::move(prop)) {}
    ExprPtr object;
    QString property;
};

// =============================================================================
// Statement kinds
// =============================================================================
class Stmt {
public:
    enum Kind {
        ExprStmt,
        DimStmt,
        AssignStmt,
        MemberAssignStmt,
        IfStmt,
        WhileStmt,
        ForStmt,
        ForEachStmt,
        ReturnStmt,
        ExitStmt,
        PrintStmt,
        BlockStmt,
    };

    explicit Stmt(Kind k, int line) : kind(k), line(line) {}
    virtual ~Stmt() = default;

    Kind kind;
    int  line;
};

class ExprStatement : public Stmt {
public:
    ExprStatement(int line, ExprPtr e) : Stmt(ExprStmt, line), expr(std::move(e)) {}
    ExprPtr expr;
};

class DimStatement : public Stmt {
public:
    DimStatement(int line, QString n, ExprPtr init = nullptr)
        : Stmt(DimStmt, line), name(std::move(n)), initializer(std::move(init)) {}
    QString name;
    ExprPtr initializer;     // optional
};

class AssignStatement : public Stmt {
public:
    AssignStatement(int line, QString n, ExprPtr v)
        : Stmt(AssignStmt, line), name(std::move(n)), value(std::move(v)) {}
    QString name;
    ExprPtr value;
};

// Property assignment:  obj.field = value
class MemberAssignStatement : public Stmt {
public:
    MemberAssignStatement(int line, ExprPtr obj, QString prop, ExprPtr v)
        : Stmt(MemberAssignStmt, line), object(std::move(obj)),
          property(std::move(prop)), value(std::move(v)) {}
    ExprPtr object;
    QString property;
    ExprPtr value;
};

class BlockStatement : public Stmt {
public:
    BlockStatement(int line, QVector<StmtPtr> ss)
        : Stmt(BlockStmt, line), stmts(std::move(ss)) {}
    QVector<StmtPtr> stmts;
};

class IfStatement : public Stmt {
public:
    IfStatement(int line) : Stmt(IfStmt, line) {}
    struct Branch { ExprPtr cond; QVector<StmtPtr> body; };
    QVector<Branch>  branches;     // primary + ElseIfs
    QVector<StmtPtr> elseBody;     // empty if no Else
};

class WhileStatement : public Stmt {
public:
    WhileStatement(int line, ExprPtr c, QVector<StmtPtr> b)
        : Stmt(WhileStmt, line), cond(std::move(c)), body(std::move(b)) {}
    ExprPtr          cond;
    QVector<StmtPtr> body;
};

class ForStatement : public Stmt {
public:
    ForStatement(int line) : Stmt(ForStmt, line) {}
    QString          var;
    ExprPtr          start;
    ExprPtr          end;
    ExprPtr          step;        // optional; default = 1
    QVector<StmtPtr> body;
};

// For Each <var> In <collection> ... Next
class ForEachStatement : public Stmt {
public:
    ForEachStatement(int line) : Stmt(ForEachStmt, line) {}
    QString          var;
    ExprPtr          collection;
    QVector<StmtPtr> body;
};

class ReturnStatement : public Stmt {
public:
    ReturnStatement(int line, ExprPtr v = nullptr)
        : Stmt(ReturnStmt, line), value(std::move(v)) {}
    ExprPtr value;
};

class ExitStatement : public Stmt {
public:
    enum What { ExitSub, ExitFunction, ExitFor, ExitWhile, ExitDo };
    ExitStatement(int line, What w) : Stmt(ExitStmt, line), what(w) {}
    What what;
};

class PrintStatement : public Stmt {
public:
    PrintStatement(int line, ExprPtr e) : Stmt(PrintStmt, line), expr(std::move(e)) {}
    ExprPtr expr;       // optional — null = print blank line
};

// =============================================================================
// Sub / Function declaration  (top-level units in a compilation unit)
// =============================================================================
class SubDecl {
public:
    QString          name;
    QStringList      params;
    QVector<StmtPtr> body;
    bool             isFunction { false };       // true = Function, false = Sub
    int              line { 0 };
};

// A whole .aba/.frm code section parses into a Program: one SubDecl per
// Sub/Function definition + a list of top-level statements (declarations
// like Dim x outside subs become module-level globals).
class Program {
public:
    QVector<SubPtr>  subs;
    QVector<StmtPtr> topLevel;       // module-init statements (Dim, ...)
};

} // namespace nx

#endif // NEXOR_STUDIO_LANG_AST_H
