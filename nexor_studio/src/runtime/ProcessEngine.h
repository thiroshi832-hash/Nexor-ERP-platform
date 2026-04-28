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
#include <functional>

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
};

#endif // NEXOR_STUDIO_PROCESSENGINE_H
