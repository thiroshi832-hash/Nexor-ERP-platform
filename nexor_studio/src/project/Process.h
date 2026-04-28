// =============================================================================
// Process — a process activity definition (.prc file).
//
// A Process is a directed, ordered list of Steps that the runtime executes
// against the Project's entity store.  Each Step carries a tiny piece of
// Nexor source that runs when the step is reached:
//
//     Sub Body()
//        Print "step body"
//     End Sub
//
// The Step's `next` attribute names the step that should execute after Body
// returns.  A step of type "Final" terminates the process.
//
// Phase 7 keeps the engine synchronous (Server / Final steps only); Choice
// steps and HumanTask suspend/resume come in 7b.
//
// Disk layout:  processes/<id>/<id>.prc
// =============================================================================
#ifndef NEXOR_STUDIO_PROCESS_H
#define NEXOR_STUDIO_PROCESS_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QVector>

struct ProcessMeta {
    QString   title;
    QString   id;
    QString   description;
    QString   author;
    QDateTime created;
};

// One step in the process.
//
//   type   — "Server"  : executes `code` synchronously, then jumps to next
//            "Final"   : executes `code` (optional) and terminates
//   nextId — id of the step to run next; ignored for Final
struct StepSpec {
    QString id;
    QString type    { "Server" };
    QString nextId;
    QString code;                              // Nexor source executed for the step
};

class Process {
public:
    Process();
    explicit Process(const ProcessMeta &meta);

    const ProcessMeta &meta() const     { return m_meta; }
    void  setMeta(const ProcessMeta &m) { m_meta = m; }

    QString filePath() const                  { return m_filePath; }
    void    setFilePath(const QString &p)     { m_filePath = p; }

    QVector<StepSpec>&       steps()        { return m_steps; }
    const QVector<StepSpec>& steps() const  { return m_steps; }

    // The "Start" step is the first one in the list (or the one named "Start"
    // if such an id exists).  Returns -1 if none.
    int startIndex() const;
    int indexOfStep(const QString &id) const;

    // XML I/O — serialises {meta, steps} into a .prc file.
    bool save() const;
    bool load();

private:
    ProcessMeta         m_meta;
    QString             m_filePath;
    QVector<StepSpec>   m_steps;
};

#endif // NEXOR_STUDIO_PROCESS_H
