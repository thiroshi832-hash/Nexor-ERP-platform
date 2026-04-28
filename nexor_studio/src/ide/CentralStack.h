// =============================================================================
// CentralStack — QStackedWidget for the main content area.
// =============================================================================
#ifndef NEXOR_STUDIO_CENTRALSTACK_H
#define NEXOR_STUDIO_CENTRALSTACK_H

#include <QStackedWidget>

class WelcomePage;
class CodeEditor;
class EditorView;
class FormCanvas;
class DesignerView;
class SheetEditor;
class ProcessEditor;

class CentralStack : public QStackedWidget {
    Q_OBJECT
public:
    enum Page { PageWelcome = 0, PageEditor, PageDesigner, PageSheet, PageProcess, PageBuild, PageDebug };

    explicit CentralStack(QWidget *parent = nullptr);

    WelcomePage  *welcomePage()  const { return m_welcome; }
    CodeEditor   *codeEditor()   const;          // shortcut: editorView()->editor()
    EditorView   *editorView()   const { return m_editorView; }
    DesignerView *designerView() const { return m_designerView; }
    FormCanvas   *formCanvas()   const;
    SheetEditor  *sheetEditor()  const { return m_sheetEditor; }
    ProcessEditor *processEditor() const { return m_processEditor; }

    void showPage(Page p) { setCurrentIndex(p); }

private:
    WelcomePage  *m_welcome      { nullptr };
    EditorView   *m_editorView   { nullptr };
    DesignerView *m_designerView { nullptr };
    SheetEditor  *m_sheetEditor  { nullptr };
    ProcessEditor *m_processEditor { nullptr };
};

#endif // NEXOR_STUDIO_CENTRALSTACK_H
