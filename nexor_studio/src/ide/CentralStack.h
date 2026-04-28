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

class CentralStack : public QStackedWidget {
    Q_OBJECT
public:
    enum Page { PageWelcome = 0, PageEditor, PageDesigner, PageBuild, PageDebug };

    explicit CentralStack(QWidget *parent = nullptr);

    WelcomePage  *welcomePage()  const { return m_welcome; }
    CodeEditor   *codeEditor()   const;          // shortcut: editorView()->editor()
    EditorView   *editorView()   const { return m_editorView; }
    DesignerView *designerView() const { return m_designerView; }
    FormCanvas   *formCanvas()   const;

    void showPage(Page p) { setCurrentIndex(p); }

private:
    WelcomePage  *m_welcome      { nullptr };
    EditorView   *m_editorView   { nullptr };
    DesignerView *m_designerView { nullptr };
};

#endif // NEXOR_STUDIO_CENTRALSTACK_H
