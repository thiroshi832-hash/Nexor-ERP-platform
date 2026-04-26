// =============================================================================
// CentralStack — QStackedWidget for the main content area: WelcomePage,
// EditorPlaceholder, DesignerPlaceholder, BuildPanel, DebugPanel.
// (Editor & Designer are placeholders in this initial scaffold; they will
//  be filled out in feature branches.)
// =============================================================================
#ifndef NEXOR_STUDIO_CENTRALSTACK_H
#define NEXOR_STUDIO_CENTRALSTACK_H

#include <QStackedWidget>

class WelcomePage;

class CentralStack : public QStackedWidget {
    Q_OBJECT
public:
    enum Page { PageWelcome, PageEditor, PageDesigner, PageBuild, PageDebug };

    explicit CentralStack(QWidget *parent = nullptr);

    WelcomePage *welcomePage() const { return m_welcome; }

    void showPage(Page p) { setCurrentIndex(p); }

private:
    WelcomePage *m_welcome;
};

#endif // NEXOR_STUDIO_CENTRALSTACK_H
