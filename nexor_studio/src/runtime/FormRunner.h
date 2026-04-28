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

class FormRunner {
public:
    using OutputFn = std::function<void(const QString &)>;

    // Loads filePath, builds a QDialog, shows it.  Returns true on success.
    // out / err are optional sinks for Print and runtime errors.
    static bool runForm(const QString &filePath,
                        QWidget *parent  = nullptr,
                        OutputFn out     = nullptr,
                        OutputFn err     = nullptr);
};

#endif // NEXOR_STUDIO_FORMRUNNER_H
