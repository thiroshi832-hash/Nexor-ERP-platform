// =============================================================================
// EditorView — VB6-style code editor wrapper.
//
//   ┌─────────────────────────────────────┬────────────────────────────────┐
//   │ Object: [(General) ▾]               │ Procedure: [(declarations) ▾]  │
//   ├─────────────────────────────────────┴────────────────────────────────┤
//   │  CodeEditor (line numbers + Nexor syntax highlight)                  │
//   └──────────────────────────────────────────────────────────────────────┘
//
// Object dropdown lists "(General)", "Form", and every widget on the canvas.
// Procedure dropdown shows the events for the picked object.  Choosing a
// procedure scrolls to that handler — or appends a stub if none exists.
//
// Designed to be a drop-in stand-in for the bare CodeEditor inside
// CentralStack: editor() returns the inner CodeEditor so existing call
// sites keep working.
// =============================================================================
#ifndef NEXOR_STUDIO_EDITORVIEW_H
#define NEXOR_STUDIO_EDITORVIEW_H

#include <QWidget>

class CodeEditor;
class FormCanvas;
class QComboBox;

class EditorView : public QWidget {
    Q_OBJECT
public:
    explicit EditorView(QWidget *parent = nullptr);

    CodeEditor *editor() const { return m_editor; }

    // Optional — only used to populate the Object dropdown when the editor
    // is in form mode.  Listening to its modified() signal keeps the list
    // in sync as widgets are added / renamed / deleted.
    void setFormCanvas(FormCanvas *canvas);

    // Re-read the current state of editor + canvas and rebuild dropdowns.
    void refresh();

signals:
    // Same shape as PropertyPanel's signal; MainWindow wires the same slot.
    void eventHandlerRequested(const QString &targetName,
                               const QString &eventName);

private slots:
    void onObjectChanged(int index);
    void onProcedureChanged(int index);

private:
    void populateProcedures();

    QComboBox  *m_objectCombo;
    QComboBox  *m_procCombo;
    CodeEditor *m_editor;
    FormCanvas *m_canvas { nullptr };
    bool        m_updating { false };
};

#endif // NEXOR_STUDIO_EDITORVIEW_H
