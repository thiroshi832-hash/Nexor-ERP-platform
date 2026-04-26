// =============================================================================
// WelcomePage — faithful copy of Qt Creator's welcome page layout.
//
//   ┌──────────────┬──────────────────────────────────────────────────┐
//   │ ┌──────────┐ │  Sessions  [⚙ Manage]   Projects  [+ New] [▥ Open]│
//   │ │ Projects │ │                                                  │
//   │ └──────────┘ │  1 ▶ default                1 ▤ MyProject        │
//   │ ┌──────────┐ │       (last session)             C:/path/...     │
//   │ │ Examples │ │                                                  │
//   │ └──────────┘ │                            2 ▤ AnotherProject    │
//   │ ┌──────────┐ │                                                  │
//   │ │ Tutorials│ │                                                  │
//   │ └──────────┘ │                                                  │
//   │              │                                                  │
//   │ New to Nexor?│                                                  │
//   │ Learn how to │                                                  │
//   │ build apps   │                                                  │
//   │ with Nexor.  │                                                  │
//   │ ┌──────────┐ │                                                  │
//   │ │Get Start.│ │                                                  │
//   │ └──────────┘ │                                                  │
//   │              │                                                  │
//   │ ◯ Account    │                                                  │
//   │ ▭ Community  │                                                  │
//   │ ≡ Blogs      │                                                  │
//   │ ? User Guide │                                                  │
//   └──────────────┴──────────────────────────────────────────────────┘
// =============================================================================
#ifndef NEXOR_STUDIO_WELCOMEPAGE_H
#define NEXOR_STUDIO_WELCOMEPAGE_H

#include <QWidget>
#include <QStringList>

class QStackedWidget;
class QToolButton;
class QButtonGroup;
class QVBoxLayout;
class QLabel;

class WelcomePage : public QWidget {
    Q_OBJECT
public:
    enum SubTab { TabProjects = 0, TabExamples = 1, TabTutorials = 2 };

    explicit WelcomePage(QWidget *parent = nullptr);

    void setRecentProjects(const QStringList &paths);

signals:
    void newProjectRequested();
    void openProjectRequested();
    void recentProjectActivated(const QString &path);
    void exampleActivated(const QString &name);
    void getStartedRequested();

private slots:
    void onSubTabClicked(int tab);

private:
    QWidget *buildSideBar();
    QWidget *buildProjectsView();
    QWidget *buildExamplesView();
    QWidget *buildTutorialsView();

    void rebuildSessionList(QVBoxLayout *into);
    void rebuildRecentProjects();
    void rebuildExampleList(QVBoxLayout *into);
    void rebuildTutorialList(QVBoxLayout *into);

    QToolButton    *m_newBtn;
    QToolButton    *m_openBtn;
    QToolButton    *m_manageSessionsBtn;
    QToolButton    *m_getStartedBtn;
    QButtonGroup   *m_subTabGroup;
    QToolButton    *m_tabProjects;
    QToolButton    *m_tabExamples;
    QToolButton    *m_tabTutorials;

    QStackedWidget *m_contentStack;
    QVBoxLayout    *m_recentRowsLayout;   // holds dynamically-built recent rows
    QStringList     m_recentPaths;
};

#endif // NEXOR_STUDIO_WELCOMEPAGE_H
