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
#include <QPointF>
#include <QSizeF>

struct ProcessMeta {
    QString   title;
    QString   id;
    QString   description;
    QString   author;
    QDateTime created;
};

// One step in the process.
//
//   type   — "Server"    : executes `code` synchronously, then jumps to next
//            "Choice"    : executes `code`; the value the body Returns
//                          (a step id, as a String) becomes the next step
//            "HumanTask" : opens `formId` modally; on accept executes `code`
//                          then jumps to next; on reject terminates
//            "Final"     : executes `code` (optional) and terminates
//   nextId — id of the step to run next (Server / HumanTask).  For Choice it
//            is the fallback if the body returns Empty.  Ignored for Final.
//   formId — file name of the form to show for HumanTask steps (relative to
//            the project's form locations; HumanTask only).
struct StepSpec {
    QString id;
    QString type    { "Server" };
    QString nextId;
    QString formId;                            // HumanTask only
    QString code;                              // Nexor source executed for the step
    QString name;                              // human-readable label (BPMN @name)
    QPointF pos;                               // diagram position (BPMN DI)
    QSizeF  size;                              // diagram size     (BPMN DI)

    // For Choice (exclusiveGateway) steps: outgoing branches.  Each branch
    // is the value the body Returns paired with the target step id.  If the
    // body's return doesn't match any branch the engine falls back to the
    // step's `nextId`.
    struct Branch {
        QString returnValue;       // matches Function's Return
        QString targetId;          // step id to jump to
        QString label;             // arrow label in the diagram
    };
    QVector<Branch> branches;
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

    // I/O — BPMN 2.0 XML is the canonical format (see project/BpmnIo.h).
    // load()/save() detect the file format from the root element:
    //   <NexorPackage>        legacy .prc — auto-converted on load
    //   <bpmn:definitions>    BPMN 2.0
    bool save() const;
    bool load();

    // Returns true if this process is held in a .bpmn file (or about to be —
    // i.e., the path ends in .bpmn).  Used by Studio when picking the
    // designer to open.
    bool isBpmn() const;

private:
    ProcessMeta         m_meta;
    QString             m_filePath;
    QVector<StepSpec>   m_steps;
};

#endif // NEXOR_STUDIO_PROCESS_H
