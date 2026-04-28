#include "Environment.h"

namespace nx {

Environment::Environment(std::shared_ptr<Environment> parent)
    : m_parent(std::move(parent)) {}

void Environment::define(const QString &name, const Value &v) {
    m_vars.insert(normalize(name), v);
}

bool Environment::has(const QString &name) const {
    QString n = normalize(name);
    if (m_vars.contains(n)) return true;
    return m_parent ? m_parent->has(name) : false;
}

Value Environment::get(const QString &name) const {
    QString n = normalize(name);
    auto it = m_vars.find(n);
    if (it != m_vars.end()) return it.value();
    return m_parent ? m_parent->get(name) : Value();
}

void Environment::assign(const QString &name, const Value &v) {
    QString n = normalize(name);
    // Walk parents looking for an existing binding.
    Environment *cur = this;
    while (cur) {
        auto it = cur->m_vars.find(n);
        if (it != cur->m_vars.end()) { it.value() = v; return; }
        cur = cur->m_parent.get();
    }
    // Not found — VB semantics: implicitly create in the outermost scope.
    Environment *out = this;
    while (out->m_parent) out = out->m_parent.get();
    out->m_vars.insert(n, v);
}

} // namespace nx
