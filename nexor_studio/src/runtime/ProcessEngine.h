// =============================================================================
// ProcessEngine — runs a Process activity (.prc) synchronously.
//
// Each step's <code> block is compiled into a synthetic Sub named
// `__Step_<id>` on a fresh NexorRuntime; the engine then walks the steps
// starting from the Start step, calling each sub and jumping to its `next`
// until it either hits a Final step or runs out of next-pointers.
//
// Phase 7: synchronous (Server / Final only).  Choice steps and HumanTask
// suspend/resume come in Phase 7b.
// =============================================================================
#ifndef NEXOR_STUDIO_PROCESSENGINE_H
#define NEXOR_STUDIO_PROCESSENGINE_H

#include <QString>
#include <QHash>
#include <functional>
#include "language/Value.h"

class Process;
class Project;

namespace nx { class NexorRuntime; }

class ProcessEngine {
public:
    using OutputFn = std::function<void(const QString &)>;

    // Runs the process referenced by absolute .prc path.
    // out / err are optional sinks for Print and runtime errors.
    // project may be null (no entity binding); when non-null sheets are
    // registered against the runtime so step code can use entities.
    static bool runProcess(const QString &prcFilePath,
                           OutputFn out = nullptr,
                           OutputFn err = nullptr,
                           const Project *project = nullptr);

    // Runs an already-loaded Process (e.g. straight from the editor without
    // touching disk).  Useful for "Run" while the user has unsaved tweaks.
    static bool runProcess(const Process &process,
                           OutputFn out = nullptr,
                           OutputFn err = nullptr,
                           const Project *project = nullptr);

    // ── Server-friendly persistent execution (Phase 16) ─────────────────
    //
    // Suspendable engine: HumanTask steps return control to the caller
    // instead of blocking on a modal form.  Core's PackageApi uses these
    // entry points to drive process_instances persisted in SQLite.
    //
    // RunState tells the caller what the engine just did:
    //   Completed     - the process reached a Final step normally
    //   AwaitingHuman - pending a HumanTask; awaitingFormId is set,
    //                   awaitingStepId names the step we're paused on,
    //                   vars carries the live var bag for resume
    //   Failed        - error during execution; lastError is populated
    enum class RunState { Completed, AwaitingHuman, Failed };

    // Vars are shipped as a flat string->Value bag so the caller can
    // serialise to JSON / SQLite without touching the language internals.
    using Vars = QHash<QString, nx::Value>;

    struct StepResult {
        RunState  state          { RunState::Completed };
        QString   awaitingFormId;
        QString   awaitingStepId;
        Vars      vars;
        QString   lastError;
    };

    // Run from `startStepId` (or the canonical start when empty) with the
    // given var bag.  Stops when:
    //   - we hit a Final step                 -> Completed
    //   - we hit a HumanTask step             -> AwaitingHuman (paused
    //                                             BEFORE running the body)
    //   - any other error                     -> Failed
    static StepResult runHeadless(const Process &process,
                                  const QString &startStepId,
                                  const Vars    &vars,
                                  OutputFn out = nullptr,
                                  OutputFn err = nullptr,
                                  const Project *project = nullptr);
};

#endif // NEXOR_STUDIO_PROCESSENGINE_H
