// =============================================================================
// CentralStack — QStackedWidget for the main content area.
//
// Pages:
//   PageWelcome  — WelcomePage
//   PageEditor   — CodeEditor
//   PageDesigner — DesignerView (WidgetPalette + FormDesigner)
//   PageBuild    — placeholder
//   PageDebug    — placeholder
// =============================================================================
#ifndef NEXOR_STUDIO_CENTRALSTACK_H
#define NEXOR_STUDIO_CENTRALSTACK_H

#include <QStackedWidget>

class WelcomePage;
class CodeEditor;
class FormDesigner;
class DesignerView;

class CentralStack : public QStackedWidget {
    Q_OBJECT
public:
    enum Page { PageWelcome = 0, PageEditor, PageDesigner, PageBuild, PageDebug };

    explicit CentralStack(QWidget *parent = nullptr);

    WelcomePage  *welcomePage()  const { return m_welcome; }
    CodeEditor   *codeEditor()   const { return m_editor; }
    DesignerView *designerView() const { return m_designerView; }
    FormDesigner *formDesigner() const;   // shortcut: m_designerView->formDesigner()

    void showPage(Page p) { setCurrentIndex(p); }

private:
    WelcomePage  *m_welcome      { nullptr };
    CodeEditor   *m_editor       { nullptr };
    DesignerView *m_designerView { nullptr };
};

#endif // NEXOR_STUDIO_CENTRALSTACK_H
