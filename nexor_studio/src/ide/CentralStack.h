// =============================================================================
// CentralStack — QStackedWidget for the main content area.
//
// Pages:
//   PageWelcome  — WelcomePage (Qt-Creator-style start page)
//   PageEditor   — CodeEditor (.aba code, syntax-highlighted, line numbers)
//   PageDesigner — FormDesigner (.frm canvas)
//   PageBuild    — placeholder for build configuration
//   PageDebug    — placeholder for debug pane
// =============================================================================
#ifndef NEXOR_STUDIO_CENTRALSTACK_H
#define NEXOR_STUDIO_CENTRALSTACK_H

#include <QStackedWidget>

class WelcomePage;
class CodeEditor;
class FormDesigner;

class CentralStack : public QStackedWidget {
    Q_OBJECT
public:
    enum Page { PageWelcome = 0, PageEditor, PageDesigner, PageBuild, PageDebug };

    explicit CentralStack(QWidget *parent = nullptr);

    WelcomePage  *welcomePage()  const { return m_welcome; }
    CodeEditor   *codeEditor()   const { return m_editor; }
    FormDesigner *formDesigner() const { return m_designer; }

    void showPage(Page p) { setCurrentIndex(p); }

private:
    WelcomePage  *m_welcome  { nullptr };
    CodeEditor   *m_editor   { nullptr };
    FormDesigner *m_designer { nullptr };
};

#endif // NEXOR_STUDIO_CENTRALSTACK_H
