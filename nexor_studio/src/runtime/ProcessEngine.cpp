#include "ProcessEngine.h"
#include "project/Process.h"
#include "project/Project.h"
#include "project/Activity.h"
#include "language/NexorRuntime.h"
#include "language/Interpreter.h"
#if defined(NEXOR_HAS_WIDGETS)
#  include "runtime/FormRunner.h"
#endif

#include <QDir>
#include <QFileInfo>
#include <QHash>

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
        // Choice steps reuse the Function form so the body's `Return <id>`
        // surfaces as the call's return value.  Server / Final / HumanTask
        // bodies use Sub.  The synthetic name is identical so call() works
        // the same in both cases.
        bool isChoice = st.type.compare("Choice", Qt::CaseInsensitive) == 0;
        out += QString("%1 __Step_%2()\n")
                  .arg(isChoice ? "Function" : "Sub", st.id);
        out += st.code;
        if (!st.code.endsWith('\n')) out += '\n';
        out += isChoice ? "End Function\n\n" : "End Sub\n\n";
    }
    return out;
}

// Resolve the form referenced by a HumanTask step.  Searches:
//   1. Absolute / project-rooted path as given
//   2. The same directory as the .prc file (processes/<id>/)
//   3. Each activity directory in the project (where forms live)
//
// Returns an empty string on failure.
QString resolveFormPath(const QString &formId,
                        const QString &prcDir,
                        const Project *project) {
    if (formId.isEmpty()) return QString();

    // Append .frm if the user typed a bare id.
    QString candidate = formId;
    if (!candidate.endsWith(".frm", Qt::CaseInsensitive)) candidate += ".frm";

    QFileInfo asAbs(candidate);
    if (asAbs.isAbsolute() && asAbs.exists()) return asAbs.absoluteFilePath();

    if (!prcDir.isEmpty()) {
        QString p = QDir(prcDir).absoluteFilePath(candidate);
        if (QFileInfo::exists(p)) return p;
    }

    if (project) {
        QString root = project->rootDir();
        // Project-relative path
        if (!root.isEmpty()) {
            QString p = QDir(root).absoluteFilePath(candidate);
            if (QFileInfo::exists(p)) return p;
        }
        // Walk activities/<id>/ directories
        for (const auto &act : project->atomicActivities()) {
            QString actDir = QFileInfo(act->filePath()).absolutePath();
            QString p = QDir(actDir).absoluteFilePath(candidate);
            if (QFileInfo::exists(p)) return p;
        }
    }
    return QString();
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

    // Per-instance Vars dictionary lives for the lifetime of this run.  Step
    // bodies see it as the magic identifier `Vars`.
    nx::Interpreter::VarStore vars;
    rt.interpreter()->setVarStore(&vars);

    QString prcDir = QFileInfo(p.filePath()).absolutePath();

    // Walk Start → next → … until Final, dangling, or rejected HumanTask.
    int safety = 1024;     // cycle guard
    while (idx >= 0 && idx < p.steps().size() && safety-- > 0) {
        const StepSpec &st = p.steps().at(idx);
        QString sub = "__Step_" + st.id;
        QString lower = st.type.toLower();

        if (lower == "humantask") {
#if defined(NEXOR_HAS_FORM_RUNNER)
            QString formPath = resolveFormPath(st.formId, prcDir, project);
            if (formPath.isEmpty()) {
                if (err) err(QString("HumanTask step '%1' references unknown form '%2'.")
                                 .arg(st.id, st.formId));
                return false;
            }
            bool accepted = FormRunner::runFormModal(formPath, nullptr,
                                                     out, err, project);
#else
            // Synchronous engine on a host without FormRunner (e.g. Core).
            // HumanTask can't open a modal form here; the caller should be
            // using runHeadless() instead.
            if (err) err(QString("HumanTask in synchronous engine without FormRunner - "
                                  "use runHeadless on this host."));
            return false;
            bool accepted = false;
            (void)accepted;
#endif
            if (!accepted) {
                if (out) out(QString("Process: HumanTask '%1' rejected — aborting.")
                                 .arg(st.id));
                return true;       // graceful early-out, not an error
            }
            // After the form closes, run the body (if any) so it can finalise
            // server-side state in Vars / entities.
            if (rt.hasSub(sub)) {
                rt.call(sub);
                if (rt.hadError()) return false;
            }
            if (st.nextId.isEmpty()) return true;
            idx = p.indexOfStep(st.nextId);
            if (idx < 0) {
                if (err) err(QString("Step '%1' references missing next step '%2'.")
                                .arg(st.id, st.nextId));
                return false;
            }
            continue;
        }

        if (lower == "choice") {
            if (!rt.hasSub(sub)) {
                if (err) err(QString("Choice step '%1' has no body.").arg(st.id));
                return false;
            }
            nx::Value rv = rt.call(sub);
            if (rt.hadError()) return false;
            QString returnValue = rv.toText().trimmed();
            QString target;

            // BPMN-style: Choice has labelled outgoing branches. Use the
            // body's return value as the branch key.
            if (!st.branches.isEmpty()) {
                for (const auto &b : st.branches) {
                    if (b.returnValue.compare(returnValue, Qt::CaseInsensitive) == 0) {
                        target = b.targetId;
                        break;
                    }
                }
                if (target.isEmpty()) target = st.nextId;       // fallback
            } else {
                // Legacy / .prc semantics: the return value IS the step id.
                target = returnValue.isEmpty() ? st.nextId : returnValue;
            }
            if (target.isEmpty()) return true;                  // implicit terminate
            int next = p.indexOfStep(target);
            if (next < 0) {
                if (err) err(QString("Choice step '%1' returned unknown target '%2'.")
                                .arg(st.id, target));
                return false;
            }
            idx = next;
            continue;
        }

        // Server / Final / unknown → just call the body, then advance.
        if (rt.hasSub(sub)) {
            rt.call(sub);
            if (rt.hadError()) return false;
        }
        if (lower == "final") return true;
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

ProcessEngine::StepResult
ProcessEngine::runHeadless(const Process &process,
                           const QString &startStepId,
                           const Vars    &vars,
                           OutputFn out, OutputFn err,
                           const Project *project) {
    StepResult result;

    if (process.steps().isEmpty()) {
        result.state = RunState::Failed;
        result.lastError = "Process has no steps.";
        return result;
    }
    int idx = startStepId.isEmpty() ? process.startIndex()
                                    : process.indexOfStep(startStepId);
    if (idx < 0) {
        result.state = RunState::Failed;
        result.lastError = "Unknown start step: " + startStepId;
        return result;
    }

    nx::NexorRuntime rt;
    if (out) rt.setOutput(out);
    if (err) rt.setError(err);
    if (project) rt.registerProjectSheets(project);

    QString unit = process.filePath().isEmpty()
        ? QString("<process:%1>").arg(process.meta().id)
        : QFileInfo(process.filePath()).fileName();
    QString src;
    {
        // Mirror buildModuleSource - private to this TU but trivial enough
        // to inline here so we don't rip the helper out of the anonymous
        // namespace.
        for (const StepSpec &st : process.steps()) {
            bool isChoice = st.type.compare("Choice", Qt::CaseInsensitive) == 0;
            src += QString("%1 __Step_%2()\n")
                       .arg(isChoice ? "Function" : "Sub", st.id);
            src += st.code;
            if (!st.code.endsWith('\n')) src += '\n';
            src += isChoice ? "End Function\n\n" : "End Sub\n\n";
        }
    }
    if (!rt.compile(src, unit)) {
        result.state = RunState::Failed;
        result.lastError = "compile error: " + rt.lastError();
        return result;
    }

    // Seed the var store with the caller's bag.
    nx::Interpreter::VarStore store;
    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it)
        store.insert(it.key().toLower(), it.value());
    rt.interpreter()->setVarStore(&store);

    int safety = 1024;
    while (idx >= 0 && idx < process.steps().size() && safety-- > 0) {
        const StepSpec &st = process.steps().at(idx);
        QString sub = "__Step_" + st.id;
        QString lower = st.type.toLower();

        if (lower == "humantask") {
            // Suspend BEFORE running the body.  The body runs after the
            // user closes the form and the caller resumes us.
            result.state          = RunState::AwaitingHuman;
            result.awaitingStepId = st.id;
            result.awaitingFormId = st.formId;
            // Snapshot var store back out for persistence.
            for (auto it = store.constBegin(); it != store.constEnd(); ++it)
                result.vars.insert(it.key(), it.value());
            return result;
        }

        if (lower == "choice") {
            if (!rt.hasSub(sub)) {
                result.state = RunState::Failed;
                result.lastError = QString("Choice step '%1' has no body.").arg(st.id);
                return result;
            }
            nx::Value rv = rt.call(sub);
            if (rt.hadError()) {
                result.state = RunState::Failed;
                result.lastError = rt.lastError();
                return result;
            }
            QString returnValue = rv.toText().trimmed();
            QString target;
            if (!st.branches.isEmpty()) {
                for (const auto &b : st.branches) {
                    if (b.returnValue.compare(returnValue, Qt::CaseInsensitive) == 0) {
                        target = b.targetId;
                        break;
                    }
                }
                if (target.isEmpty()) target = st.nextId;
            } else {
                target = returnValue.isEmpty() ? st.nextId : returnValue;
            }
            if (target.isEmpty()) break;          // implicit terminate
            int next = process.indexOfStep(target);
            if (next < 0) {
                result.state = RunState::Failed;
                result.lastError =
                    QString("Choice step '%1' returned unknown target '%2'.")
                        .arg(st.id, target);
                return result;
            }
            idx = next;
            continue;
        }

        // Server / Final / unknown - run the body, then advance.
        if (rt.hasSub(sub)) {
            rt.call(sub);
            if (rt.hadError()) {
                result.state = RunState::Failed;
                result.lastError = rt.lastError();
                return result;
            }
        }
        if (lower == "final") break;
        if (st.nextId.isEmpty()) break;
        idx = process.indexOfStep(st.nextId);
        if (idx < 0) {
            result.state = RunState::Failed;
            result.lastError =
                QString("Step '%1' references missing next step '%2'.")
                    .arg(st.id, st.nextId);
            return result;
        }
    }

    result.state = RunState::Completed;
    for (auto it = store.constBegin(); it != store.constEnd(); ++it)
        result.vars.insert(it.key(), it.value());
    return result;
}
