// =============================================================================
// MainWindow — Nexor Studio top-level window.
//
//   ┌──────────────────────────────────────────────────────────────────┐
//   │ Menu bar: File / Edit / View / Build / Run / Debug / Help        │
//   ├──┬─────────────┬─────────────────────────────────────────────────┤
//   │S │ Project     │                                                 │
//   │i │ tree (dock) │   CentralStack                                  │
//   │d │             │   (Welcome / Editor / Designer / Build / Debug) │
//   │e │             │                                                 │
//   │  │             ├─────────────────────────────────────────────────┤
//   │B │             │   Output dock (build / debug console)           │
//   │a │             │                                                 │
//   │r │             │                                                 │
//   ├──┴─────────────┴─────────────────────────────────────────────────┤
//   │ Status bar                                                       │
//   └──────────────────────────────────────────────────────────────────┘
// =============================================================================
#ifndef NEXOR_STUDIO_MAINWINDOW_H
#define NEXOR_STUDIO_MAINWINDOW_H

#include <QMainWindow>
#include <memory>

class SideBar;
class CentralStack;
class ProjectTree;
class Project;
class QPlainTextEdit;
class QDockWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onNewProject();
    void onOpenProject();
    void onCloseProject();
    void onAbout();

    void onModeChanged(int mode);

    void onNewAtomicActivity();
    void onNewProcessActivity();
    void onNewSheet();

    void onFormActivated(const QString &absPath);
    void onActivityActivated(const QString &absPath);

private:
    void setupUi();
    void buildMenus();
    void appendOutput(const QString &line, const QString &color = QString());
    void loadProjectFromFile(const QString &path);
    void rememberRecent(const QString &path);
    QStringList loadRecentProjects() const;

    SideBar      *m_sideBar;
    CentralStack *m_central;
    ProjectTree  *m_projectTree;
    QDockWidget  *m_projectDock;
    QDockWidget  *m_outputDock;
    QPlainTextEdit *m_output;

    std::unique_ptr<Project> m_project;
};

#endif // NEXOR_STUDIO_MAINWINDOW_H
