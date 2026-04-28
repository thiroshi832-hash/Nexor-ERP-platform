// =============================================================================
// Parser — recursive-descent parser for Nexor.
//
// Grammar (informal):
//   program     := { topItem }
//   topItem     := subDecl | funcDecl | stmt
//   subDecl     := [Public|Private] "Sub"  Ident [ "(" params ")" ] NL stmts "End" "Sub"
//   funcDecl    := [Public|Private] "Function" Ident [ "(" params ")" ] [ "As" Ident ] NL stmts "End" "Function"
//   stmt        := dimStmt | assign | exprStmt | ifStmt | whileStmt
//                | forStmt  | printStmt | returnStmt | exitStmt
//   dimStmt     := "Dim" Ident [ "As" Ident ] [ "=" expr ]
//   assign      := Ident "=" expr
//   ifStmt      := "If" expr "Then" NL stmts { "ElseIf" expr "Then" NL stmts } [ "Else" NL stmts ] "End" "If"
//   whileStmt   := "While" expr NL stmts "Wend"
//   forStmt     := "For" Ident "=" expr "To" expr [ "Step" expr ] NL stmts "Next" [ Ident ]
//
//   expr        := orExpr
//   orExpr      := andExpr  { "Or"  andExpr }
//   andExpr     := notExpr  { "And" notExpr }
//   notExpr     := "Not" notExpr | cmpExpr
//   cmpExpr     := concatExpr { ("=" | "<>" | "<" | "<=" | ">" | ">=") concatExpr }
//   concatExpr  := addExpr  { "&"  addExpr }
//   addExpr     := mulExpr  { ("+" | "-") mulExpr }
//   mulExpr     := unary    { ("*" | "/" | "Mod") unary }
//   unary       := "-" unary | postfix
//   postfix     := primary { "(" args ")" | "." Ident }
//   primary     := Integer | Double | String | True | False | Nothing
//                | Ident | "(" expr ")"
// =============================================================================
#ifndef NEXOR_STUDIO_LANG_PARSER_H
#define NEXOR_STUDIO_LANG_PARSER_H

#include "Token.h"
#include "Ast.h"

namespace nx {

class Parser {
public:
    explicit Parser(QVector<Token> tokens);

    Program parse();
    bool    ok()    const { return m_error.isEmpty(); }
    QString error() const { return m_error; }
    int     errorLine() const { return m_errorLine; }

private:
    // ── Token cursor helpers
    const Token& peek(int off = 0) const;
    const Token& advance();
    bool         check(TokKind k) const;
    bool         match(TokKind k);
    bool         matchAny(std::initializer_list<TokKind> kinds);
    bool         consumeNewline();           // consumes Newline OR Eof
    void         skipNewlines();

    // ── Error reporting
    void   errorAt(const Token &t, const QString &msg);
    Token  expect(TokKind k, const QString &what);

    // ── Statements
    StmtPtr parseStmt();
    StmtPtr parseDim();
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseFor();
    StmtPtr parsePrint();
    StmtPtr parseReturn();
    StmtPtr parseExit();
    StmtPtr parseAssignOrExpr();
    QVector<StmtPtr> parseStmts(std::initializer_list<TokKind> stopKinds);

    // ── Sub / Function
    SubPtr parseSub(bool isFunction);

    // ── Expressions (precedence climb)
    ExprPtr parseExpr();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseNot();
    ExprPtr parseCmp();
    ExprPtr parseConcat();
    ExprPtr parseAdd();
    ExprPtr parseMul();
    ExprPtr parseUnary();
    ExprPtr parsePostfix();
    ExprPtr parsePrimary();
    QVector<ExprPtr> parseArgs();

    QVector<Token> m_tokens;
    int            m_pos { 0 };
    QString        m_error;
    int            m_errorLine { 0 };
};

} // namespace nx

#endif // NEXOR_STUDIO_LANG_PARSER_H
