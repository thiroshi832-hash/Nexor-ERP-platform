// =============================================================================
// FormRunner — runs a .frm file as a real top-level QDialog.
//
// Uses WidgetFactory::create() — the SAME factory the designer uses — so
// the running form is byte-for-byte identical to the design.
// =============================================================================
#ifndef NEXOR_STUDIO_FORMRUNNER_H
#define NEXOR_STUDIO_FORMRUNNER_H

#include <QString>

class QWidget;

class FormRunner {
public:
    // Loads filePath, builds a QDialog, shows it modally relative to parent.
    // Returns true if the form file was readable.
    static bool runForm(const QString &filePath, QWidget *parent = nullptr);
};

#endif // NEXOR_STUDIO_FORMRUNNER_H
