// =============================================================================
// WelcomePage — Qt Creator-style welcome screen.
//
//   ┌──────────────┬──────────────────────────────────────────────────┐
//   │  + New Proj. │   Recent Projects                                │
//   │  ⇪ Open Proj.│                                                  │
//   │              │   ┌────────────────────────────────────────────┐ │
//   │  ▸ Projects  │   │  ProjectCard                               │ │
//   │    Examples  │   ├────────────────────────────────────────────┤ │
//   │    Tutorials │   │  ProjectCard                               │ │
//   │              │   └────────────────────────────────────────────┘ │
//   │              │                                                  │
//   ├──────────────┴──────────────────────────────────────────────────┤
//   │  Documentation   ·   Examples   ·   GitHub                      │
//   └─────────────────────────────────────────────────────────────────┘
//
// Sub-tabs (Projects / Examples / Tutorials) drive the right-hand content
// stack.  Recent projects render as full-width "cards" with name + path.
// =============================================================================
#ifndef NEXOR_STUDIO_WELCOMEPAGE_H
#define NEXOR_STUDIO_WELCOMEPAGE_H

#include <QWidget>

class QListWidget;
class QStackedWidget;
class QToolButton;
class QButtonGroup;

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

private slots:
    void onSubTabClicked(int tab);

private:
    QWidget *buildSideBar();
    QWidget *buildProjectsView();
    QWidget *buildExamplesView();
    QWidget *buildTutorialsView();
    QWidget *buildFooter();

    QToolButton    *m_newBtn;
    QToolButton    *m_openBtn;
    QButtonGroup   *m_tabGroup;
    QToolButton    *m_tabProjects;
    QToolButton    *m_tabExamples;
    QToolButton    *m_tabTutorials;

    QStackedWidget *m_contentStack;
    QListWidget    *m_recentList;
    QListWidget    *m_exampleList;
    QListWidget    *m_tutorialList;
};

#endif // NEXOR_STUDIO_WELCOMEPAGE_H
