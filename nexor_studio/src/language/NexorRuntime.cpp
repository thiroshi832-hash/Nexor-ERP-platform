#include "NexorRuntime.h"
#include "Lexer.h"
#include "Parser.h"
#include "project/Project.h"
#include "project/Sheet.h"

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

void NexorRuntime::registerProjectSheets(const Project *project) {
    if (!project) return;
    // Open the project's persistent store at <project_root>/project.ndb
    QString root = project->rootDir();
    if (!root.isEmpty()) {
        QString dbPath = root + "/project.ndb";
        if (!m_interp->entityStore()->isOpen())
            m_interp->entityStore()->open(dbPath);
    }
    for (const auto &sht : project->sheets()) {
        nx::SheetSchema s;
        s.sheetId = sht->meta().id;
        for (const FieldSpec &fs : sht->fields()) {
            nx::SheetSchemaField rf;
            rf.name        = fs.name;
            rf.type        = fs.type;
            rf.isKey       = fs.isKey;
            rf.required    = fs.required;
            rf.defaultText = fs.defaultText;
            s.fields.append(rf);
        }
        m_interp->registerSheet(s);
    }
}

} // namespace nx
