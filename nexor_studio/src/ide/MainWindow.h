// =============================================================================
// MainWindow — Nexor Studio top-level window (Qt Creator-style layout).
//
//   ┌──────────────────────────────────────────────────────────────────┐
//   │ Menu bar                                                         │
//   ├────┬──────────────┬──────────────────────────────────────────────┤
//   │ F  │ Project      │                                              │
//   │ a  │ panel        │   Central content (per active mode)          │
//   │ n  │ (Edit/Debug/ │                                              │
//   │ c  │  Projects    ├──────────────────────────────────────────────┤
//   │ y  │  modes only) │   Output content (stacked panes)             │
//   │ T  │              ├──────────────────────────────────────────────┤
//   │ a  │              │   1 Issues  2 Search  3 App  4 Compile  5 Dbg│
//   │ b  │              ├──────────────────────────────────────────────┤
//   │ B  │              │   Status bar                                 │
//   │ a  │              │                                              │
//   │ r  │              │                                              │
//   └────┴──────────────┴──────────────────────────────────────────────┘
//
// In Welcome mode the project panel is hidden so the welcome page fills
// the full content width — exactly like Qt Creator.
// =============================================================================
#ifndef NEXOR_STUDIO_MAINWINDOW_H
#define NEXOR_STUDIO_MAINWINDOW_H

#include <QMainWindow>
#include <memory>

class FancyTabBar;
class CentralStack;
class ProjectTree;
class OutputPane;
class Project;
class QWidget;
class QAction;
class QSplitter;
class QLabel;

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
    void onSheetActivated(const QString &absPath);
    void onProcessActivated(const QString &absPath);
    void onRunProcessRequested(const QString &absPath);

private:
    void setupUi();
    void buildMenus();
    void appendOutput(const QString &line, const QString &color = QString());
    void loadProjectFromFile(const QString &path);
    void rememberRecent(const QString &path);
    QStringList loadRecentProjects() const;
    void applyModeLayout(int mode);

    FancyTabBar    *m_tabBar;
    CentralStack   *m_central;
    ProjectTree    *m_projectTree;
    QWidget        *m_projectPanel;
    OutputPane     *m_outputPane;
    QSplitter      *m_horizontalSplit;
    QSplitter      *m_verticalSplit;
    QAction        *m_toggleProjectPanelAction;
    QAction        *m_toggleOutputPaneAction;
    QLabel         *m_statusModeLabel;

    std::unique_ptr<Project> m_project;
};

#endif // NEXOR_STUDIO_MAINWINDOW_H
