#include "NexorRuntime.h"
#include "Lexer.h"
#include "Parser.h"

namespace nx {

NexorRuntime::NexorRuntime()
    : m_interp(std::make_unique<Interpreter>()) {}

bool NexorRuntime::compile(const QString &source, const QString &unitName) {
    m_lastError.clear();
    Lexer lex(source);
    auto tokens = lex.tokenize();
    if (!lex.ok()) { m_lastError = lex.lastError(); return false; }

    Parser p(tokens);
    Program prog = p.parse();
    if (!p.ok()) { m_lastError = p.error(); return false; }

    if (!m_interp->load(prog, unitName)) {
        m_lastError = m_interp->lastError();
        return false;
    }
    return true;
}

Value NexorRuntime::call(const QString &name, const QVector<Value> &args) {
    Value v = m_interp->call(name, args);
    if (m_interp->hadError()) m_lastError = m_interp->lastError();
    return v;
}

bool NexorRuntime::hasSub(const QString &name) const {
    return m_interp->hasSub(name);
}

} // namespace nx
