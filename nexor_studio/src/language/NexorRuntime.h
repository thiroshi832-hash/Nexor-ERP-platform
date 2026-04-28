// =============================================================================
// NexorRuntime — convenience facade combining Lexer, Parser, and Interpreter.
//
//   compile(source, unitName)  -> bool   (lex + parse + register subs)
//   call(name, args)           -> Value  (invoke a registered sub)
//   setOutput / setError       -> route Print + runtime errors
// =============================================================================
#ifndef NEXOR_STUDIO_LANG_NEXORRUNTIME_H
#define NEXOR_STUDIO_LANG_NEXORRUNTIME_H

#include "Interpreter.h"
#include "EntityStore.h"
#include <QString>
#include <memory>

class Project;     // forward — Studio's project model

namespace nx {

class NexorRuntime {
public:
    NexorRuntime();

    bool compile(const QString &source, const QString &unitName = "<unit>");
    Value call(const QString &name, const QVector<Value> &args = {});
    bool  hasSub(const QString &name) const;

    // Opens the project's SQLite database (creating it if needed) and
    // registers every sheet schema.  After this call, user code can refer
    // to entities by name (Customer.Find(1) etc.) and changes persist.
    void registerProjectSheets(const Project *project);

    void setOutput(Interpreter::OutputCallback cb) { m_interp->setOutput(std::move(cb)); }
    void setError (Interpreter::OutputCallback cb) { m_interp->setError (std::move(cb)); }

    QString lastError() const { return m_lastError; }
    bool    hadError()  const { return !m_lastError.isEmpty(); }

    Interpreter *interpreter() const { return m_interp.get(); }

private:
    std::unique_ptr<Interpreter> m_interp;
    QString                      m_lastError;
};

} // namespace nx

#endif // NEXOR_STUDIO_LANG_NEXORRUNTIME_H
