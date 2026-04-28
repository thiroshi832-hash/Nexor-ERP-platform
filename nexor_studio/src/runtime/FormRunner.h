// =============================================================================
// FormRunner — runs a .frm file as a real top-level QDialog.
//
// Builds the form via WidgetFactory (so what runs == what the designer shows),
// then compiles the form's <Code> block with NexorRuntime and wires
//   • Form_Load              fired right after show()
//   • Form_Unload            fired in dialog destructor
//   • <widgetName>_Click     fired on QAbstractButton::clicked
//   • <widgetName>_Change    fired on QLineEdit::textChanged etc.
// to the appropriate interpreter calls.  Print output and runtime errors are
// emitted via the supplied callbacks (so MainWindow can pipe them to the
// OUTPUT pane).
// =============================================================================
#ifndef NEXOR_STUDIO_FORMRUNNER_H
#define NEXOR_STUDIO_FORMRUNNER_H

#include <QString>
#include <functional>

class QWidget;
class Project;

class FormRunner {
public:
    using OutputFn = std::function<void(const QString &)>;

    // Loads filePath, builds a QDialog, shows it.  Returns true on success.
    // out / err are optional sinks for Print and runtime errors.
    // project may be null (no entity binding); when non-null, the form's
    // dataSource attribute is honoured and Form.Save / .Load are wired to
    // the project's SQLite store.
    static bool runForm(const QString &filePath,
                        QWidget *parent       = nullptr,
                        OutputFn out          = nullptr,
                        OutputFn err          = nullptr,
                        const Project *project= nullptr);

    // Same shape as runForm, but the dialog runs modally (exec) and the call
    // blocks until the user closes it.  Returns true if the dialog was
    // accepted, false if it was rejected (e.g. window-close == reject).
    // Used by ProcessEngine for HumanTask steps.
    static bool runFormModal(const QString &filePath,
                             QWidget *parent       = nullptr,
                             OutputFn out          = nullptr,
                             OutputFn err          = nullptr,
                             const Project *project= nullptr);
};

#endif // NEXOR_STUDIO_FORMRUNNER_H
