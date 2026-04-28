#include "ProcessEngine.h"
#include "project/Process.h"
#include "project/Project.h"
#include "language/NexorRuntime.h"

#include <QFileInfo>

namespace {

// Builds:
//   Sub __Step_<id>()
//     <step body code>
//   End Sub
//
// for every step, in declaration order.  Step ids are assumed to be valid
// identifiers (the editor enforces letters/numbers).  The user's body is
// emitted verbatim — runtime errors quote real line numbers in the .prc.
QString buildModuleSource(const Process &p) {
    QString out;
    for (const StepSpec &st : p.steps()) {
        out += QString("Sub __Step_%1()\n").arg(st.id);
        out += st.code;
        if (!st.code.endsWith('\n')) out += '\n';
        out += "End Sub\n\n";
    }
    return out;
}

bool runLoaded(const Process &p,
               ProcessEngine::OutputFn out,
               ProcessEngine::OutputFn err,
               const Project *project,
               const QString &unitName) {
    if (p.steps().isEmpty()) {
        if (err) err("Process has no steps.");
        return false;
    }
    int idx = p.startIndex();
    if (idx < 0) {
        if (err) err("Process has no Start step.");
        return false;
    }

    nx::NexorRuntime rt;
    if (out) rt.setOutput(out);
    if (err) rt.setError(err);
    if (project) rt.registerProjectSheets(project);

    QString src = buildModuleSource(p);
    if (!rt.compile(src, unitName)) {
        if (err) err("compile error in " + unitName + ": " + rt.lastError());
        return false;
    }

    // Walk Start → next → … until Final or dangling.
    int safety = 1024;     // cycle guard
    while (idx >= 0 && idx < p.steps().size() && safety-- > 0) {
        const StepSpec &st = p.steps().at(idx);
        QString sub = "__Step_" + st.id;
        if (rt.hasSub(sub)) {
            rt.call(sub);
            if (rt.hadError()) return false;
        }
        if (st.type.compare("Final", Qt::CaseInsensitive) == 0) return true;
        if (st.nextId.isEmpty()) return true;
        idx = p.indexOfStep(st.nextId);
        if (idx < 0) {
            if (err) err(QString("Step '%1' references missing next step '%2'.")
                            .arg(st.id, st.nextId));
            return false;
        }
    }
    if (safety <= 0 && err)
        err("Process aborted: step-execution safety limit (1024 hops) reached.");
    return safety > 0;
}

} // namespace

bool ProcessEngine::runProcess(const QString &prcFilePath,
                               OutputFn out, OutputFn err,
                               const Project *project) {
    Process p;
    p.setFilePath(prcFilePath);
    if (!p.load()) {
        if (err) err("Failed to read process: " + prcFilePath);
        return false;
    }
    return runLoaded(p, std::move(out), std::move(err),
                     project, QFileInfo(prcFilePath).fileName());
}

bool ProcessEngine::runProcess(const Process &process,
                               OutputFn out, OutputFn err,
                               const Project *project) {
    QString unit = process.filePath().isEmpty()
        ? QString("<process:%1>").arg(process.meta().id)
        : QFileInfo(process.filePath()).fileName();
    return runLoaded(process, std::move(out), std::move(err), project, unit);
}
