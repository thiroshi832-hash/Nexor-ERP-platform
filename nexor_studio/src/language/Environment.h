// =============================================================================
// Environment — lexical scope for variables.  Case-insensitive on lookup.
// =============================================================================
#ifndef NEXOR_STUDIO_LANG_ENVIRONMENT_H
#define NEXOR_STUDIO_LANG_ENVIRONMENT_H

#include "Value.h"
#include <QHash>
#include <memory>

namespace nx {

class Environment : public std::enable_shared_from_this<Environment> {
public:
    explicit Environment(std::shared_ptr<Environment> parent = nullptr);

    // Declares (or redeclares) a new local in this scope.
    void define(const QString &name, const Value &v);

    // Walks parents looking for the name; returns Empty if not found.
    Value get(const QString &name) const;
    bool  has(const QString &name) const;

    // Walks parents to assign; if not found, defines in the outermost
    // (global) scope — VB semantics for implicitly-declared variables.
    void assign(const QString &name, const Value &v);

    std::shared_ptr<Environment> parent() const { return m_parent; }

private:
    static QString normalize(const QString &name) { return name.toLower(); }

    std::shared_ptr<Environment> m_parent;
    QHash<QString, Value>        m_vars;
};

} // namespace nx

#endif // NEXOR_STUDIO_LANG_ENVIRONMENT_H
